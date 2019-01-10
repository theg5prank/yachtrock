#if __APPLE__
// for SO_NOSIGPIPE
#define _DARWIN_C_SOURCE 1
#endif

#include <yachtrock/yachtrock.h>

#if YACHTROCK_MULTIPROCESS

#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <spawn.h>
#include <err.h>
#include <pthread.h>
#include <limits.h>
#include <stdint.h>
#include <sysexits.h>
#include <assert.h>
#include <unistd.h>

#include "yrutil.h"
#include "multiprocess.h"
#include "multiprocess_inferior.h"
#include "multiprocess_superior.h"

#if YR_USE_STDATOMIC

#include <stdatomic.h>
typedef volatile _Atomic unsigned long inferiority_suspension_counter_t;
#define SUSPEND_INC(counter) ((void)atomic_fetch_add(&counter, 1))
#define SUSPEND_DEC(counter) (atomic_fetch_sub(&counter, 1) - 1)
#define SUSPEND_CHECK(counter) (atomic_load(&counter) > 0)

#else

#  if YR_USE_STDTHREADS

#include <threads.h>
typedef once_flag yr_once_t;
#define YR_ONCE_INIT ONCE_FLAG_INIT
#define yr_once call_once
typedef mtx_t yr_mtx_t;
#define yr_mtx_init mtx_init
#define yr_mtx_init_ok thrd_success
#define yr_mtx_lock mtx_lock
#define yr_mtx_unlock mtx_unlock

#  else

typedef pthread_once_t yr_once_t;
#define YR_ONCE_INIT PTHREAD_ONCE_INIT
#define yr_once pthread_once
typedef pthread_mutex_t yr_mtx_t;
#define yr_mtx_init(mtx) pthread_mutex_init(mtx, NULL)
#define yr_mtx_init_ok 0
#define yr_mtx_lock pthread_mutex_lock
#define yr_mtx_unlock pthread_mutex_unlock

#  endif

typedef unsigned long inferiority_suspension_counter_t;
static yr_mtx_t suspension_counter_mtx;
static yr_once_t suspension_counter_mtx_init_once = YR_ONCE_INIT;
static void init_suspension_mtx(void)
{
  YR_RUNTIME_ASSERT(yr_mtx_init(&suspension_counter_mtx) == yr_mtx_init_ok,
                    "mutex initialization failed");
}
static inline void suspend_increment(inferiority_suspension_counter_t *counter)
{
  yr_once(&suspension_counter_mtx_init_once, init_suspension_mtx);
  yr_mtx_lock(&suspension_counter_mtx);
  ++*counter;
  yr_mtx_unlock(&suspension_counter_mtx);
}
static inline inferiority_suspension_counter_t
suspend_decrement(inferiority_suspension_counter_t *counter)
{
  yr_mtx_lock(&suspension_counter_mtx);
  inferiority_suspension_counter_t result = --*counter;
  yr_mtx_unlock(&suspension_counter_mtx);
  return result;
}
static inline bool suspend_check(inferiority_suspension_counter_t *counter)
{
  yr_mtx_lock(&suspension_counter_mtx);
  inferiority_suspension_counter_t snapshot = *counter;
  yr_mtx_unlock(&suspension_counter_mtx);
  return snapshot > 0;
}
#define SUSPEND_INC(ctr) suspend_increment(&(ctr))
#define SUSPEND_DEC(ctr) suspend_decrement(&(ctr))
#define SUSPEND_CHECK(ctr) suspend_check(&(ctr))
#endif

const struct inferior_handle YR_INFERIOR_HANDLE_NULL = {
  .pid = -1,
  .socket = -1
};

bool yr_inferior_handle_is_null(struct inferior_handle handle)
{
  assert((handle.pid == -1 && handle.socket == -1) ||
         (handle.pid != -1 && handle.socket != -1));
  return handle.pid == -1;
}

#define SOCKET_ENV_VAR "YR_INFERIOR_SOCKET"

static int socket_fd = -1;
static pthread_once_t init_socket_once = PTHREAD_ONCE_INIT;
static void init_socket_fd(void)
{
  char *fd_num_str = getenv(SOCKET_ENV_VAR);
  if ( fd_num_str ) {
    YR_RUNTIME_ASSERT(fd_num_str && *fd_num_str,
                      "bogus string for socket environment variable: \"%s\"",
                      fd_num_str ? fd_num_str : "(null)");

    char *end = NULL;
    long result = strtol(fd_num_str, &end, 10);
    YR_RUNTIME_ASSERT(*end == '\0' && result <= INT_MAX,
                      "bogus string for socket environment variable: \"%s\"", fd_num_str);
    // good number. sanity check that it is a file descriptor
    int flags = fcntl(result, F_GETFD);
    YR_RUNTIME_ASSERT(flags >= 0,
                      "socket environment variable \"%s\" did not name a file descriptor?",
                      fd_num_str);
    socket_fd = result;
  }
}
static int _yr_real_inferior_socket(void)
{
  pthread_once(&init_socket_once, init_socket_fd);
  return socket_fd;
}

static inferiority_suspension_counter_t suspension_counter = 0;

int yr_inferior_socket(void)
{
  return SUSPEND_CHECK(suspension_counter) ? -1 : _yr_real_inferior_socket();
}

bool yr_process_is_inferior(void)
{
  return !SUSPEND_CHECK(suspension_counter) && _yr_real_inferior_socket() >= 0;
}

void yr_suspend_inferiority(void)
{
  (void)_yr_real_inferior_socket();
  SUSPEND_INC(suspension_counter);
}

void yr_reset_inferiority(void)
{
  (void)SUSPEND_DEC(suspension_counter);
}

static size_t environ_count(char **environ)
{
  char **environ_arr_end = environ;
  while ( *(environ_arr_end++) );
  return environ_arr_end - environ - 1;
}

// Spawn inferior process, return socket to it and pid by reference.
// return true on success
bool yr_spawn_inferior(char *path, char **argv, char **environ,
                       struct inferior_handle *out_result)
{
  int result = -1;

  // copy environ but add one extra space at the end
  size_t environ_entries = environ_count(environ);
  char **new_environ = yr_malloc((environ_entries + 2) * sizeof(char *));
  for ( size_t i = 0; i < environ_entries + 2; i++ ) {
    new_environ[i] = NULL;
  }
  for ( size_t i = 0; i < environ_entries; i++ ) {
    char *str = environ[i];
    if ( str ) {
      new_environ[i] = yr_strdup(environ[i]);
    } else {
      break;
    }
  }

  int sockets[2] = { -1, -1 };
  if ( socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0 ) {
    yr_warn("socketpair failed");
    goto out;
  }

  for ( int i = 0; i < 2; i++ ) {
#if __APPLE__ || BSD
    int on = 1;
    if ( setsockopt(sockets[i], SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on)) ) {
      yr_warn("setsockopt failed for SO_NOSIGPIPE");
      goto out;
    }
#endif

    int flags = fcntl(sockets[i], F_GETFL);
    if ( flags == -1 ) {
      yr_warn("fcntl(F_GETFL) failed");
      goto out;
    }
    if ( fcntl(sockets[i], F_SETFL, flags | O_NONBLOCK) == -1 ) {
      yr_warn("fcntl(F_SETFL) failed");
      goto out;
    }
  }

  char **new_env_place = new_environ;
  // Find either SOCKET_ENV_VAR or the fist NULL
  for ( ; *new_env_place; new_env_place++ ) {
    char *found_var = strstr(*new_env_place, SOCKET_ENV_VAR "=");
    if ( found_var == *new_env_place ) {
      free(*new_env_place);
      *new_env_place = NULL;
      break;
    }
  }
  YR_RUNTIME_ASSERT((new_env_place - new_environ) <= (environ_entries + 1),
                    "overflow of environment");
  char _;
  char *fmt = SOCKET_ENV_VAR"=%d";
  size_t necessary = snprintf(&_, 0, fmt, sockets[1]) + 1;
  *new_env_place = yr_malloc(necessary);
  snprintf(*new_env_place, necessary, fmt, sockets[1]);

  posix_spawn_file_actions_t file_actions;
  int error = posix_spawn_file_actions_init(&file_actions);
  if ( error != 0 ) {
    yr_warnc(error, "posix_spawn_file_actions_init failed");
    goto out;
  }
  error = posix_spawn_file_actions_addclose(&file_actions, sockets[0]);
  if ( error != 0 ) {
    yr_warnc(error, "posix_spawn_file_actions_addclose failed");
    goto out;
  }

  /* If we have been invoked recursively (possible with inferiority suspension), add a close for OUR
   * inferior socket
   */
  if ( _yr_real_inferior_socket() != -1 ) {
    error = posix_spawn_file_actions_addclose(&file_actions, _yr_real_inferior_socket());
    if ( error != 0 ) {
      yr_warnc(error, "posix_spawn_file_actions_addclose failed (inferior socket)");
      goto out;
    }
  }

  pid_t pid;
  result = posix_spawnp(&pid, path, &file_actions, NULL, argv, new_environ);

  if ( result != 0 ) {
    yr_warnc(result, "posix_spawn failed");
    goto out;
  }

 out:
  if ( result == 0 && out_result ) {
    out_result->pid = pid;
    out_result->socket = sockets[0];
  }
  posix_spawn_file_actions_destroy(&file_actions);

  for ( size_t i = 0; i < environ_entries + 1; i++ ) {
    if ( new_environ[i] ) {
      free(new_environ[i]);
    }
  }
  free(new_environ);

  if ( result == -1 && sockets[0] != -1 ) {
    close(sockets[0]);
  }
  if ( sockets[1] != -1 ) {
    close(sockets[1]);
  }
  return result == 0;
}

bool yr_recv_length(int sock, void *buf, size_t len, struct timeval *timeout, int flags)
{
  static const size_t drainbuf_len = 0x1000;
  bool drain = (flags & RECV_LENGTH_DRAIN) != 0;
  char drainbuf[drain ? drainbuf_len : 1];

  if ( drain ) {
    buf = drainbuf;
    len = drainbuf_len;
  }

  size_t received = 0;
  while ( received < len ) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(sock, &set);
    int sel_result = select(sock + 1, &set, NULL, NULL, timeout);
    if ( sel_result < 0 ) {
      yr_warn("select failed");
      return false;
    } else if ( sel_result == 0 ) {
      yr_warnx("select timed out");
      return false;
    }
    assert(FD_ISSET(sock, &set));
    ssize_t receive_iter;
    EINTR_RETRY(receive_iter = recv(sock, ((char *)buf) + received, len - received, 0));
    if ( receive_iter < 0 ) {
      yr_warn("recv failed");
      return false;
    } else if ( receive_iter == 0 ) {
      if ( drain ) {
        return true;
      } else {
        yr_warnx("recv returned zero bytes");
        return false;
      }
    }

    if ( !drain ) {
      received += receive_iter;
    }
  }
  return true;
}

bool yr_send_length(int sock, const void *buf, size_t len, struct timeval *timeout)
{
  size_t sent = 0;
  while ( sent < len ) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(sock, &set);
    int sel_result = select(sock + 1, NULL, &set, NULL, timeout);
    if ( sel_result < 0 ) {
      yr_warn("select failed");
      return false;
    } else if ( sel_result == 0 ) {
      yr_warnx("select timed out");
      return false;
    }
    assert(FD_ISSET(sock, &set));
    int flags = 0;
#if !__APPLE__ && !defined(BSD)
    flags |= MSG_NOSIGNAL;
#endif
    ssize_t send_iter;
    EINTR_RETRY(send_iter = send(sock, ((const char *)buf) + sent, len - sent, flags));
    if ( send_iter < 0 ) {
      yr_warn("send failed");
      return false;
    } else if ( send_iter == 0 ) {
      yr_warnx("send sent zero bytes?");
      return false;
    }
    sent += send_iter;
  }
  return true;
}

static uint32_t netbuf_to_uint32(const char netbuf[static 4])
{
  uint32_t result = 0;
  const unsigned char *ubuf = (unsigned char *)netbuf;
  result |= (ubuf[0] << 3);
  result |= (ubuf[1] << 2);
  result |= (ubuf[2] << 1);
  result |= (ubuf[3] << 0);
  return result;
}

static void uint32_to_netbuf(uint32_t in, char netbuf[static 4])
{
  unsigned char *ubuf = (unsigned char *)netbuf;
  ubuf[0] = (in >> 3) & 0xff;
  ubuf[1] = (in >> 2) & 0xff;
  ubuf[2] = (in >> 1) & 0xff;
  ubuf[3] = (in >> 0) & 0xff;
}

bool yr_recv_uint32(int sock, uint32_t out[static 1], struct timeval *timeout)
{
  char buf[4];
  bool ok = yr_recv_length(sock, buf, sizeof(buf), timeout, 0);
  if ( !ok ) {
    return false;
  }
  *out = netbuf_to_uint32(buf);
  return true;
}

bool yr_send_uint32(int sock, uint32_t in, struct timeval *timeout)
{
  char buf[4];
  uint32_to_netbuf(in, buf);
  return yr_send_length(sock, buf, sizeof(buf), timeout);
}

bool yr_send_message(int sock, struct yr_message *in, struct timeval *timeout)
{
  bool ok = yr_send_uint32(sock, in->message_code, timeout);
  ok = ok && yr_send_uint32(sock, in->payload_length, timeout);
  if ( in->payload_length ) {
    ok = ok && yr_send_length(sock, in->payload, in->payload_length, timeout);
  }
  return ok;
}

bool yr_recv_message(int sock, struct yr_message **out_message, struct timeval *timeout)
{
  uint32_t message_code = 0;
  bool ok = yr_recv_uint32(sock, &message_code, timeout);
  uint32_t payload_length = 0;
  ok = ok && yr_recv_uint32(sock, &payload_length, timeout);
  struct yr_message *message = NULL;
  if ( ok ) {
    message = yr_malloc(sizeof(struct yr_message) + payload_length);
    message->message_code = message_code;
    message->payload_length = payload_length;
    ok = ok && yr_recv_length(sock, message->payload, payload_length, timeout, 0);
    if ( ok ) {
      *out_message = message;
    } else {
      free(message);
    }
  }
  return ok;
}

static bool size_is_32bit(size_t size)
{
  return ((unsigned long long)size & 0xFFFFFFFF00000000ULL) == 0ULL;
}

static bool testcase_sizes_are_32bit(yr_test_case_t suite)
{
  return size_is_32bit(strlen(suite->name));
}

static bool testsuite_sizes_are_32bit(yr_test_suite_t suite)
{
  if ( !size_is_32bit(suite->num_cases) || !size_is_32bit(strlen(suite->name)) ) {
    return false;
  }
  for ( size_t i = 0; i < suite->num_cases; i++ ) {
    if ( !testcase_sizes_are_32bit(&(suite->cases[i])) ) {
      return false;
    }
  }
  return true;
}

static bool collection_sizes_are_32bit(yr_test_suite_collection_t collection)
{
  if ( !size_is_32bit(collection->num_suites) ) {
    return false;
  }
  for ( size_t i = 0; i < collection->num_suites; i++ ) {
    if ( !testsuite_sizes_are_32bit(collection->suites[i]) ) {
      return false;
    }
  }
  return true;
}

struct yr_message *yr_message_create_with_payload(enum yr_inferior_message message,
                                                  void *payload, size_t payload_length)
{
  size_t objlen = sizeof(struct yr_message) + payload_length;
  struct yr_message *result = yr_malloc(objlen);
  result->message_code = message;
  result->payload_length = payload_length;
  if ( payload_length ) {
    memcpy(result->payload, payload, payload_length);
  }
  return result;
}


#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

static void write_data_to_buf(const char *data, size_t datalen, char *buf, size_t buflen, size_t *total)
{
  size_t available = *total < buflen ? buflen - *total : 0;
  size_t to_write = MIN(datalen, available);
  if ( to_write > 0 ) {
    memcpy(buf + *total, data, to_write);
  }
  *total += datalen;
}

static void write_uint32_to_buf(uint32_t num, char *buf, size_t buflen, size_t *total)
{
  char numbuf[4];
  uint32_to_netbuf(num, numbuf);
  write_data_to_buf(numbuf, sizeof(numbuf), buf, buflen, total);
}

size_t yr_multiprocess_collection_desc(char *buf, size_t buflen, yr_test_suite_collection_t collection)
{
  size_t total = 0;
  uint32_t num_suites = collection->num_suites;
  write_uint32_to_buf(num_suites, buf, buflen, &total);
  for ( size_t suite_i = 0; suite_i < collection->num_suites; suite_i++ ) {
    yr_test_suite_t suite = collection->suites[suite_i];
    write_uint32_to_buf((uint32_t)strlen(suite->name), buf, buflen, &total);
    write_data_to_buf(suite->name, strlen(suite->name), buf, buflen, &total);
    write_uint32_to_buf((uint32_t)suite->num_cases, buf, buflen, &total);
    for ( size_t case_i = 0; case_i < suite->num_cases; case_i++ ) {
      yr_test_case_t testcase = &suite->cases[case_i];
      write_uint32_to_buf((uint32_t)strlen(testcase->name), buf, buflen, &total);
      write_data_to_buf(testcase->name, strlen(testcase->name), buf, buflen, &total);
    }
  }
  return total;
}

size_t yr_invoke_case_payload(char *buf, size_t buflen, size_t suiteid, size_t caseid)
{
  size_t total = 0;
  write_uint32_to_buf(suiteid, buf, buflen, &total);
  write_uint32_to_buf(caseid, buf, buflen, &total);
  return total;
}

bool yr_extract_ids_from_invoke_case_message(struct yr_message *message, size_t *suiteid,
                                             size_t *caseid)
{
  if ( message->payload_length != 8 || message->message_code != MESSAGE_INVOKE_CASE ) {
    return false;
  }

  *suiteid = netbuf_to_uint32(message->payload);
  *caseid = netbuf_to_uint32(message->payload + 4);
  return true;
}

size_t yr_case_finished_payload(char *buf, size_t buflen, size_t suiteid, size_t caseid)
{
  size_t total = 0;
  write_uint32_to_buf(suiteid, buf, buflen, &total);
  write_uint32_to_buf(caseid, buf, buflen, &total);
  return total;
}

bool yr_extract_ids_from_case_finished_message(struct yr_message *message, size_t *suiteid,
                                               size_t *caseid)
{
  if ( message->payload_length != 8 || message->message_code != MESSAGE_CASE_FINISHED ) {
    return false;
  }

  *suiteid = netbuf_to_uint32(message->payload);
  *caseid = netbuf_to_uint32(message->payload + 4);
  return true;
}

size_t yr_case_result_payload(char *buf, size_t buflen, size_t suiteid, size_t caseid,
                              yr_result_t result)
{
  char result_byte = (char)result;
  size_t total = 0;
  write_uint32_to_buf(suiteid, buf, buflen, &total);
  write_uint32_to_buf(caseid, buf, buflen, &total);
  write_data_to_buf(&result_byte, 1, buf, buflen, &total);
  return total;
}

extern bool yr_extract_info_from_case_result_message(struct yr_message *message, size_t *suiteid,
                                                     size_t *caseid, yr_result_t *result)
{
  if ( message->payload_length != 9 || message->message_code != MESSAGE_CASE_RESULT ) {
    return false;
  }

  *suiteid = netbuf_to_uint32(message->payload);
  *caseid = netbuf_to_uint32(message->payload + 4);
  *result = *(message->payload + 8);
  return true;
}

void
yr_run_suite_collection_under_store_multiprocess(char *path, char **argv, char **environ,
                                                 yr_test_suite_collection_t collection,
                                                 yr_result_store_t store,
                                                 struct yr_runtime_callbacks runtime_callbacks)
{
  if ( yr_process_is_inferior() ) {
    yr_inferior_checkin(collection, runtime_callbacks);
    abort();
  }

  if ( !collection_sizes_are_32bit(collection) ) {
    yr_warnx("%s: Collection sizes are not expressible in 32 bits. "
          "That's too big! Not running suites.", __FUNCTION__);
    return;
  }

  yr_handle_run_multiprocess(path, argv, environ, collection, store, runtime_callbacks);
}

void
yr_inferior_checkin(yr_test_suite_collection_t collection,
                    struct yr_runtime_callbacks runtime_callbacks)
{
  YR_RUNTIME_ASSERT(yr_process_is_inferior(), "only inferior process may call %s", __FUNCTION__);
  if ( !collection_sizes_are_32bit(collection) ) {
    yr_errx(EX_DATAERR, "%s: inferior: Collection sizes are not expressible in 32 bits. "
            "That's too big! aborting.", __FUNCTION__);
  }
  yr_inferior_loop(collection, runtime_callbacks);
}

#endif // YACHTROCK_MULTIPROCESS

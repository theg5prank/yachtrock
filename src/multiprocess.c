#if __APPLE__
// for SO_NOSIGPIPE
#define _DARWIN_C_SOURCE 1
#endif

#include <yachtrock/yachtrock.h>

#if YACHTROCK_POSIXY

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

#include "yrutil.h"

/*
 * Message format: msg_code[1] payload_len[4] payload[payload_len]
 */
enum yr_inferior_message {
  /* Payload format: {} */
  MESSAGE_REQUEST_COLLECTION_DESC,
  /* Payload format: num_suites[4] (suite * num_suites)
   * suite: suite_name_len[4], suite_name[suite_len], num_cases[4], (testcase * num_cases)
   * testcase: testcase_name_len[4], testcase_name[testcase_name_len]
   */
  MESSAGE_PROVIDE_COLLECTION_DESC,
  /* Payload format: suiteid[4], caseid[4] */
  MESSAGE_INVOKE_CASE,
  /* Payload format: suiteid[4], caseid[4], result[1] */
  MESSAGE_CASE_RESULT,
  /* Payload format: {} */
  MESSAGE_TERMINATE,
};

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
static int inferior_socket(void)
{
  pthread_once(&init_socket_once, init_socket_fd);
  return socket_fd;
}

bool yr_process_is_inferior(void)
{
  return inferior_socket() >= 0;
}

static size_t environ_count(char **environ)
{
  char **environ_arr_end = environ;
  while ( *(environ_arr_end++) );
  return environ_arr_end - environ - 1;
}

// Spawn inferior process, return socket to it and pid by reference.
// return 0 on success
struct spawn_result { pid_t pid; int socket; };
static int spawn_inferior(char *path, char **argv, char **environ,
                          struct spawn_result *out_result)
{
  int result = -1;

  // copy environ but add one extra space at the end
  size_t environ_entries = environ_count(environ);
  char **new_environ = malloc((environ_entries + 2) * sizeof(char *));
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
    warn("socketpair failed");
    goto out;
  }

  for ( int i = 0; i < 2; i++ ) {
#if __APPLE__ || BSD
    int on = 1;
    if ( setsockopt(sockets[i], SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on)) ) {
      warn("setsockopt failed for SO_NOSIGPIPE");
      goto out;
    }
#endif

    int flags = fcntl(sockets[i], F_GETFL);
    if ( flags == -1 ) {
      warn("fcntl(F_GETFL) failed");
      goto out;
    }
    if ( fcntl(sockets[i], F_SETFL, flags | O_NONBLOCK) == -1 ) {
      warn("fcntl(F_SETFL) failed");
      goto out;
    }
  }

  char **new_env_place = new_environ;
  while ( *++new_env_place );
  YR_RUNTIME_ASSERT((new_env_place - new_environ) <= (environ_entries + 1),
                    "overflow of environment");
  char _;
  char *fmt = SOCKET_ENV_VAR"=%d";
  size_t necessary = snprintf(&_, 0, fmt, sockets[1]) + 1;
  *new_env_place = malloc(necessary);
  snprintf(*new_env_place, necessary, fmt, sockets[1]);

  posix_spawn_file_actions_t file_actions;
  int error = posix_spawn_file_actions_init(&file_actions);
  if ( error != 0 ) {
    warnc(error, "posix_spawn_file_actions_init failed");
    goto out;
  }
  error = posix_spawn_file_actions_addclose(&file_actions, sockets[0]);
  if ( error != 0 ) {
    warnc(error, "posix_spawn_file_actions_addclose failed");
    goto out;
  }

  pid_t pid;
  result = posix_spawnp(&pid, path, &file_actions, NULL, argv, new_environ);

  if ( result != 0 ) {
    warnc(result, "posix_spawn failed");
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
  return result;
}

enum recv_length_flags {
  RECV_LENGTH_DRAIN = 1 << 0
};


static bool recv_length(int sock, void *buf, size_t len, struct timeval *timeout, int flags)
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
      warn("select failed");
      return false;
    } else if ( sel_result == 0 ) {
      warnx("select timed out");
      return false;
    }
    assert(FD_ISSET(sock, &set));
    ssize_t receive_iter = recv(sock, ((char *)buf) + received, len - received, 0);
    if ( receive_iter < 0 ) {
      warn("recv failed");
      return false;
    } else if ( receive_iter == 0 ) {
      if ( drain ) {
        return true;
      } else {
        warnx("recv returned zero bytes");
        return false;
      }
    }

    if ( !drain ) {
      received += receive_iter;
    }
  }
  return true;
}

static bool send_length(int sock, const void *buf, size_t len, struct timeval *timeout)
{
  size_t sent = 0;
  while ( sent < len ) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(sock, &set);
    int sel_result = select(sock + 1, NULL, &set, NULL, timeout);
    if ( sel_result < 0 ) {
      warn("select failed");
      return false;
    } else if ( sel_result == 0 ) {
      warnx("select timed out");
      return false;
    }
    assert(FD_ISSET(sock, &set));
    int flags = 0;
#if !__APPLE__ && !defined(BSD)
    flags |= MSG_NOSIGNAL;
#endif
    ssize_t send_iter = send(sock, ((const char *)buf) + sent, len - sent, flags);
    if ( send_iter < 0 ) {
      warn("send failed");
      return false;
    } else if ( send_iter == 0 ) {
      warnx("send sent zero bytes?");
      return false;
    }
    sent += send_iter;
  }
  return true;
}

static bool recv_uint32(int sock, uint32_t out[static 1], struct timeval *timeout)
{
  char buf[4];
  bool ok = recv_length(sock, buf, sizeof(buf), timeout, 0);
  if ( !ok ) {
    return false;
  }
  uint32_t tmp;
  memcpy(&tmp, buf, sizeof(buf));
  *out = ntohl(tmp);
  return true;
}

static bool send_uint32(int sock, uint32_t in, struct timeval *timeout)
{
  uint32_t tmp = htonl(in);
  return send_length(sock, &tmp, sizeof(tmp), timeout);
}

static bool inferior_handle_command(int sock, uint32_t command,
                                    yr_test_suite_collection_t collection,
                                    struct yr_runtime_callbacks runtime_callbacks)
{
  switch ( command ) {
  case MESSAGE_TERMINATE:
    warnx("inferior terminating cleanly per superior instruction");
    exit(0);
  default:
    warnx("unknown command %ld", (long)command);
    return false;
  }
}

static void inferior_loop(yr_test_suite_collection_t collection,
                          struct yr_runtime_callbacks runtime_callbacks)
{
  int sock = inferior_socket();
  while ( 1 ) {
    uint32_t command;
    bool ok = recv_uint32(sock, &command, NULL);
    if ( !ok ) {
      errx(EX_SOFTWARE, "inferior exiting due to error");
    }
    ok = inferior_handle_command(sock, command, collection, runtime_callbacks);
    if ( !ok ) {
      errx(EX_SOFTWARE, "failed to handle command");
    }
  }
}

static void drain_inferior_best_effort(struct spawn_result inferior_info)
{
  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  char buf[4096];
  bool drained = recv_length(inferior_info.socket, buf, sizeof(buf), &timeout, RECV_LENGTH_DRAIN);
  // if we drained the inferior is on the exit path so we can wait
  int wait_flags = drained ? 0 : WNOHANG;
  int stat_loc;
  int result = waitpid(inferior_info.pid, &stat_loc, wait_flags);
  if ( result == 0 ) {
    warnx("inferior not dead yet, forcibly killing");
    kill(inferior_info.pid, SIGKILL);
    result = waitpid(inferior_info.pid, &stat_loc, 0);
  }
  if ( result < 0 ) {
    warn("waitpid failed");
  }
}

static bool spawn_inferior_checking_collection(char *path, char **argv, char **environ,
                                               yr_test_suite_collection_t collection,
                                               struct spawn_result *out_result)
{
  struct spawn_result tmp_spawn_result;
  bool ok = !spawn_inferior(path, argv, environ, &tmp_spawn_result);

  ok = ok && send_uint32(tmp_spawn_result.socket, MESSAGE_TERMINATE, NULL);

  if ( ok && out_result ) {
    *out_result = tmp_spawn_result;
  }

  return ok;
}

void
yr_run_suite_collection_under_store_multiprocess(char *path, char **argv, char **environ,
                                                 yr_test_suite_collection_t collection,
                                                 yr_result_store_t store,
                                                 struct yr_runtime_callbacks runtime_callbacks)
{
  if ( yr_process_is_inferior() ) {
    inferior_loop(collection, runtime_callbacks);
    abort(); // unreachable
  }
  struct spawn_result spawn_result;
  bool ok = spawn_inferior_checking_collection(path, argv, environ, collection, &spawn_result);

  if ( !ok ) {
    warnx("failed to spawn and check collection");
  }

  // if we're still OK at this point, be a good boy and attempt to reap our child
  if ( ok ) {
    drain_inferior_best_effort(spawn_result);
  }
}

#endif // YACHTROCK_POSIXY

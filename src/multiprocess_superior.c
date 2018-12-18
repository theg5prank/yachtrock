#include <yachtrock/yachtrock.h>

#if YACHTROCK_POSIXY

#include <sys/wait.h>
#include <err.h>
#include <signal.h>
#include <assert.h>

#include "multiprocess_superior.h"

static void drain_inferior_best_effort(struct inferior_handle inferior_info)
{
  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  char buf[4096];
  bool drained = yr_recv_length(inferior_info.socket, buf, sizeof(buf), &timeout, RECV_LENGTH_DRAIN);
  // if we drained the inferior is on the exit path so we can wait
  int wait_flags = drained ? 0 : WNOHANG;
  int stat_loc = 0;
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

static void forcibly_terminate_inferior(struct inferior_handle inferior)
{
  kill(inferior.pid, SIGKILL);
  int stat_loc = 0;
  int result = waitpid(inferior.pid, &stat_loc, 0);
  if ( result < 0 ) {
    warn("waitpid failed");
  }
}

static bool send_term_message(struct inferior_handle inferior)
{
  struct yr_message request_collection_message = {
    .message_code = MESSAGE_TERMINATE,
    .payload_length = 0
  };
  return yr_send_message(inferior.socket, &request_collection_message, NULL);
}

static bool check_collection(struct inferior_handle inferior,
                             yr_test_suite_collection_t collection)
{
  struct yr_message request_collection_message = {
    .message_code = MESSAGE_REQUEST_COLLECTION_DESC,
    .payload_length = 0
  };
  bool ok = yr_send_message(inferior.socket, &request_collection_message, NULL);

  size_t collection_len = yr_multiprocess_collection_desc(NULL, 0, collection);
  char *collection_desc = malloc(collection_len);
  size_t filled_out_size = yr_multiprocess_collection_desc(collection_desc, collection_len,
                                                           collection);
  assert(filled_out_size == collection_len);

  struct yr_message *desc_message;
  ok = ok && yr_recv_message(inferior.socket, &desc_message, NULL);
  if ( !ok ) {
    assert(!desc_message);
    return false;
  }
  assert(desc_message);
  if ( desc_message->message_code != MESSAGE_PROVIDE_COLLECTION_DESC ||
       desc_message->payload_length != collection_len ||
       memcmp(desc_message->payload, collection_desc, collection_len) != 0 ) {
    return false;
  }

  return true;
}

void yr_handle_run_multiprocess(char *path, char **argv, char **environ,
                                yr_test_suite_collection_t collection,
                                yr_result_store_t store,
                                struct yr_runtime_callbacks runtime_callbacks)
{
  struct inferior_handle inferior;
  bool ok = yr_spawn_inferior(path, argv, environ, &inferior);
  ok = ok && check_collection(inferior, collection);

  if ( !ok ) {
    warnx("failed to spawn and check collection");
  }

  ok = ok && send_term_message(inferior);

  // if we're still OK at this point, be a good boy and attempt to reap our child
  if ( ok ) {
    drain_inferior_best_effort(inferior);
  } else {
    forcibly_terminate_inferior(inferior);
  }
  close(inferior.socket);
}

#endif // YACHTROCK_POSIXY

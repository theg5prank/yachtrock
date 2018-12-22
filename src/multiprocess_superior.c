#include <yachtrock/yachtrock.h>

#if YACHTROCK_POSIXY

#include <sys/wait.h>
#include <err.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>

#include "multiprocess_superior.h"
#include "yrutil.h"

static bool result_valid(yr_result_t result)
{
  switch ( result ) {
  case YR_RESULT_UNSET:
    // TODO: should this be false? Inferior should not send this.
  case YR_RESULT_PASSED:
  case YR_RESULT_FAILED:
  case YR_RESULT_SKIPPED:
    return true;
  }
  return false;
}

static int eintr_waitpid(pid_t pid, int *stat_loc, int flags)
{
  int result;
  do {
    result = waitpid(pid, stat_loc, flags);
  } while ( result < 0 && errno == EINTR );
  return result;
}

static void drain_inferior_best_effort(struct inferior_handle *inferior_info)
{
  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  char buf[4096];
  bool drained = yr_recv_length(inferior_info->socket, buf, sizeof(buf), &timeout, RECV_LENGTH_DRAIN);
  // if we drained the inferior is on the exit path so we can wait
  int wait_flags = drained ? 0 : WNOHANG;
  int stat_loc = 0;
  int result = eintr_waitpid(inferior_info->pid, &stat_loc, wait_flags);
  if ( result == 0 ) {
    yr_warnx("inferior not dead yet, forcibly killing");
    kill(inferior_info->pid, SIGKILL);
    result = eintr_waitpid(inferior_info->pid, &stat_loc, 0);
  }
  if ( result < 0 ) {
    yr_warn("waitpid failed");
  }
  close(inferior_info->socket);
  *inferior_info = YR_INFERIOR_HANDLE_NULL;
}

static void forcibly_terminate_inferior(struct inferior_handle *inferior)
{
  kill(inferior->pid, SIGKILL);
  int stat_loc = 0;
  int result = waitpid(inferior->pid, &stat_loc, 0);
  if ( result < 0 ) {
    yr_warn("waitpid failed");
  }
  close(inferior->socket);
  *inferior = YR_INFERIOR_HANDLE_NULL;
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

  size_t collection_len = 0;
  size_t filled_out_size = 0;
  char *collection_desc = NULL;
  struct yr_message *desc_message = NULL;
  
  bool ok = yr_send_message(inferior.socket, &request_collection_message, NULL);

  if ( !ok ) {
    goto out;
  }

  collection_len = yr_multiprocess_collection_desc(NULL, 0, collection);
  collection_desc = malloc(collection_len);
  filled_out_size = yr_multiprocess_collection_desc(collection_desc, collection_len,
                                                           collection);
  assert(filled_out_size == collection_len);

  ok = ok && yr_recv_message(inferior.socket, &desc_message, NULL);
  if ( !ok ) {
    assert(!desc_message);
    goto out;
  }
  assert(desc_message);
  if ( desc_message->message_code != MESSAGE_PROVIDE_COLLECTION_DESC ||
       desc_message->payload_length != collection_len ||
       memcmp(desc_message->payload, collection_desc, collection_len) != 0 ) {
    ok = false;
    goto out;
  }

 out:
  free(collection_desc);
  free(desc_message);
  return ok;
}

static bool spawn_and_check_collection(char *path, char **argv, char **environ,
                                       yr_test_suite_collection_t collection,
                                       struct inferior_handle *inferior)
{
  struct inferior_handle local_inferior;
  bool ok = yr_spawn_inferior(path, argv, environ, &local_inferior);
  if ( ok ) {
    ok = check_collection(local_inferior, collection);
    if ( !ok ) {
      yr_warnx("failed to check collection with inferior");
      forcibly_terminate_inferior(&local_inferior);
    }
  } else {
    yr_warnx("failed to spawn inferior");
  }

  if ( ok ) {
    *inferior = local_inferior;
  }

  return ok;
}

static bool spawn_inferior_if_necessary(char *path, char **argv, char **environ,
                                        yr_test_suite_collection_t collection,
                                        struct inferior_handle *inferior)
{
  if ( !yr_inferior_handle_is_null(*inferior) ) {
    return true;
  }

  struct inferior_handle new_inferior = YR_INFERIOR_HANDLE_NULL;
  bool ok = spawn_and_check_collection(path, argv, environ, collection, &new_inferior);
  if ( ok ) {
    *inferior = new_inferior;
  }
  return ok;
}

static bool invoke_case(struct inferior_handle inferior, size_t suite_index, size_t case_index,
                        yr_result_store_t case_store)
{
  size_t payload_len = yr_invoke_case_payload(NULL, 0, suite_index, case_index);
  char payload_buf[payload_len];
  yr_invoke_case_payload(payload_buf, payload_len, suite_index, case_index);

  struct yr_message *message = yr_message_create_with_payload(MESSAGE_INVOKE_CASE, payload_buf, payload_len);
  bool ok = yr_send_message(inferior.socket, message, NULL);
  free(message);
  message = NULL;

  if ( ok ) {
    do {
      free(message);
      message = NULL;
      ok = yr_recv_message(inferior.socket, &message, NULL);
      if ( ok ) {
        switch ( message->message_code ) {
        case MESSAGE_CASE_RESULT: {
          size_t result_suiteid = 0, result_caseid = 0;
          yr_result_t result = YR_RESULT_UNSET;
          ok = yr_extract_info_from_case_result_message(message, &result_suiteid, &result_caseid, &result);
          if ( !ok ) {
            yr_warnx("couldn't parse case result message");
          } else if ( result_suiteid != suite_index || result_caseid != case_index ||
                      !result_valid(result) ) {
            yr_warnx("bogus case result message: %zu %zu %d", result_suiteid, result_caseid, (int)result);
            ok = false;
          } else {
            yr_result_store_record_result(case_store, result);
          }
          break;
        }
        case MESSAGE_CASE_FINISHED: {
          size_t finished_suiteid = 0, finished_caseid = 0;
          ok = yr_extract_ids_from_case_finished_message(message, &finished_suiteid, &finished_caseid);
          if ( !ok ) {
            yr_warnx("couldn't parse case finished message");
          } else if ( finished_suiteid != suite_index || finished_caseid != case_index ) {
            yr_warnx("bogus case finished message: %zu %zu", finished_suiteid, finished_caseid);
            ok = false;
          }
          break;
        }
        default:
          yr_warnx("unknown message from inferior %d", message->message_code);
          ok = false;
          break;
        }
      }
    } while ( ok && message->message_code != MESSAGE_CASE_FINISHED );
    free(message);
    message = NULL;
  }

  if ( !ok ) {
    yr_result_store_record_result(case_store, YR_RESULT_FAILED);
  }

  return ok;
}

static bool run_collection(char *path, char **argv, char **environ,
                           yr_test_suite_collection_t collection,
                           yr_result_store_t store)
{
  struct inferior_handle inferior = YR_INFERIOR_HANDLE_NULL;

  bool spawn_failure = false;

  for ( size_t suite_index = 0; suite_index < collection->num_suites; suite_index++ ) {
    yr_test_suite_t suite = collection->suites[suite_index];
    yr_result_store_t suite_store = yr_result_store_open_subresult(store, suite->name);

    for ( size_t case_index = 0; case_index <  suite->num_cases; case_index++ ) {
      yr_test_case_t test_case = &suite->cases[case_index];
      yr_result_store_t case_store = yr_result_store_open_subresult(suite_store, test_case->name);
      if ( spawn_failure ) {
        yr_result_store_record_result(case_store, YR_RESULT_SKIPPED);
      } else {
        bool spawned_ok = spawn_inferior_if_necessary(path, argv, environ, collection, &inferior);
        if ( !spawned_ok ) {
          yr_warnx("failure spawning inferior and checking test suite collection!");
          yr_result_store_record_result(case_store, YR_RESULT_SKIPPED);
          spawn_failure = true;
          inferior = YR_INFERIOR_HANDLE_NULL;
        } else {
          bool invocation_success = invoke_case(inferior, suite_index, case_index, case_store);
          if ( !invocation_success ) {
            forcibly_terminate_inferior(&inferior);
          }
        }
      }
      yr_result_store_close(case_store);
    }
    yr_result_store_close(suite_store);
  }

  if ( !spawn_failure ) {
    if ( !yr_inferior_handle_is_null(inferior) ) {
      bool sent_ok = send_term_message(inferior);
      if ( sent_ok ) {
        drain_inferior_best_effort(&inferior);
      } else {
        forcibly_terminate_inferior(&inferior);
      }
      assert(yr_inferior_handle_is_null(inferior));
    }
  } else {
    assert(yr_inferior_handle_is_null(inferior));
  }

  return !spawn_failure;
}

void yr_handle_run_multiprocess(char *path, char **argv, char **environ,
                                yr_test_suite_collection_t collection,
                                yr_result_store_t store,
                                struct yr_runtime_callbacks runtime_callbacks)
{
  run_collection(path, argv, environ, collection, store);
}

#endif // YACHTROCK_POSIXY

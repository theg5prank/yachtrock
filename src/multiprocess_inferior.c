#include <yachtrock/yachtrock.h>

#if YACHTROCK_POSIXY

#include <err.h>
#include <stdint.h>
#include <sysexits.h>
#include <assert.h>

#include "multiprocess_inferior.h"

struct inferior_state {
  yr_test_suite_t opened_suite;
};

static bool send_collection_desc_message(int sock, yr_test_suite_collection_t collection)
{
  size_t collection_desc_size = yr_multiprocess_collection_desc(NULL, 0, collection);
  struct yr_message *message = malloc(sizeof(struct yr_message) + collection_desc_size);
  size_t filled_out_size = yr_multiprocess_collection_desc(message->payload,
                                                           collection_desc_size,
                                                           collection);
  assert(filled_out_size == collection_desc_size);
  message->message_code = MESSAGE_PROVIDE_COLLECTION_DESC;
  message->payload_length = collection_desc_size;
  bool ok = yr_send_message(sock, message, NULL);
  free(message);
  return ok;
}

static void prepare_to_run_case(yr_test_suite_t suite,
                                yr_test_case_t testcase,
                                struct inferior_state *state)
{
  assert(suite == testcase->suite);
  if ( state->opened_suite != suite ) {
    if ( state->opened_suite && state->opened_suite->lifecycle.teardown_suite ) {
      state->opened_suite->lifecycle.teardown_suite(state->opened_suite);
    }
    if ( suite->lifecycle.setup_suite ) {
      suite->lifecycle.setup_suite(suite);
    }
    state->opened_suite = suite;
  }
  if ( testcase->suite->lifecycle.setup_case ) {
    testcase->suite->lifecycle.setup_case(testcase);
  }
}

static void finish_running_case(yr_test_case_t testcase,
                                struct inferior_state *state)
{
  if ( testcase->suite->lifecycle.teardown_case ) {
    testcase->suite->lifecycle.teardown_case(testcase);
  }
}

static void finalize_state(struct inferior_state *state)
{
  if ( state->opened_suite && state->opened_suite->lifecycle.teardown_suite ) {
    state->opened_suite->lifecycle.teardown_suite(state->opened_suite);
  }
  state->opened_suite = NULL;
}

struct inferior_runtime_context {
  struct yr_runtime_callbacks provided_runtime_callbacks;
  size_t suite_index;
  size_t case_index;
  int sock;
  yr_result_t current_result;
};

static bool send_result_update(int sock, size_t suiteid, size_t caseid, yr_result_t result)
{
  size_t payload_len = yr_case_result_payload(NULL, 0, suiteid, caseid, result);
  char payload_buf[payload_len];
  yr_case_result_payload(payload_buf, payload_len, suiteid, caseid, result);
  struct yr_message *update_message = yr_message_create_with_payload(MESSAGE_CASE_RESULT,
                                                                     payload_buf,
                                                                     payload_len);
  return yr_send_message(sock, update_message, NULL);
}

static void note_potential_new_result(struct inferior_runtime_context *context,
                                      yr_result_t potential_new_result)
{
  yr_result_t real_new_result = yr_merge_result(context->current_result, potential_new_result);
  if ( real_new_result != context->current_result ) {
    bool ok = send_result_update(context->sock, context->suite_index,
                                 context->case_index, real_new_result);
    if ( !ok ) {
      errx(EX_SOFTWARE, "failure sending result update");
    }
    context->current_result = real_new_result;
  }
}

static void inferior_runtime_assertion_failure(const char *assertion, const char *file,
                                               size_t line, const char *funname, const char *s,
                                               va_list ap, void *refcon)
{
  struct inferior_runtime_context *context = refcon;
  struct yr_runtime_callbacks provided_runtime_callbacks = context->provided_runtime_callbacks;
  provided_runtime_callbacks.note_assertion_failed(assertion, file, line, funname, s, ap,
                                                   provided_runtime_callbacks.refcon);
  note_potential_new_result(context, YR_RESULT_FAILED);
}

static void inferior_runtime_test_skipped(const char *file, size_t line, const char *funname,
                                          const char *reason, va_list ap, void *refcon)
{
  struct inferior_runtime_context *context = refcon;
  struct yr_runtime_callbacks provided_runtime_callbacks = context->provided_runtime_callbacks;
  provided_runtime_callbacks.note_skipped(file, line, funname, reason, ap,
                                          provided_runtime_callbacks.refcon);
  note_potential_new_result(context, YR_RESULT_SKIPPED);
}

static bool handle_invoke_case(int sock, struct yr_message *invoke_case_message,
                               yr_test_suite_collection_t collection,
                               struct yr_runtime_callbacks runtime_callbacks,
                               struct inferior_state *state)
{
  size_t suiteid = 0, caseid = 0;
  if ( !yr_extract_ids_from_invoke_case_message(invoke_case_message, &suiteid, &caseid) ||
       (suiteid > collection->num_suites) ||
       (caseid > collection->suites[suiteid]->num_cases) ) {
    errx(EX_SOFTWARE, "bogus message payload length from superior");
  }

  struct inferior_runtime_context runtime_context = {
    .provided_runtime_callbacks = runtime_callbacks,
    .suite_index = suiteid,
    .case_index = caseid,
    .sock = sock,
    .current_result = YR_RESULT_UNSET
  };

  struct yr_runtime_callbacks inferior_runtime_callbacks = {
    .refcon = &runtime_context,
    .note_assertion_failed = inferior_runtime_assertion_failure,
    .note_skipped = inferior_runtime_test_skipped
  };

  yr_test_suite_t suite = collection->suites[suiteid];
  yr_test_case_t testcase = &suite->cases[caseid];
  struct yr_runtime_callbacks old_callbacks = yr_set_runtime_callbacks(inferior_runtime_callbacks);
  prepare_to_run_case(suite, testcase, state);
  testcase->testcase(testcase);
  finish_running_case(testcase, state);
  yr_set_runtime_callbacks(old_callbacks);
  note_potential_new_result(&runtime_context, YR_RESULT_PASSED);

  size_t payload_len = yr_case_finished_payload(NULL, 0, suiteid, caseid);
  char payload_buf[payload_len];
  yr_case_finished_payload(payload_buf, payload_len, suiteid, caseid);
  struct yr_message *case_finished_message = yr_message_create_with_payload(MESSAGE_CASE_FINISHED,
                                                                            payload_buf,
                                                                            payload_len);
  bool responded_ok = yr_send_message(sock, case_finished_message, NULL);

  return responded_ok;
}

static bool inferior_handle_message(int sock, struct yr_message *command_message,
                                    yr_test_suite_collection_t collection,
                                    struct yr_runtime_callbacks runtime_callbacks,
                                    struct inferior_state *state)
{
  switch ( command_message->message_code ) {
  case MESSAGE_REQUEST_COLLECTION_DESC:
    return send_collection_desc_message(sock, collection);
    break;
  case MESSAGE_TERMINATE:
    finalize_state(state);
    warnx("inferior terminating cleanly per superior instruction");
    exit(0);
  case MESSAGE_INVOKE_CASE:
    return handle_invoke_case(sock, command_message, collection, runtime_callbacks, state);
    break;
  default:
    warnx("unknown command %ld", (long)command_message->message_code);
    return false;
  }
}

void yr_inferior_loop(yr_test_suite_collection_t collection,
                      struct yr_runtime_callbacks runtime_callbacks)
{
  struct inferior_state state;
  state.opened_suite = NULL;
  int sock = yr_inferior_socket();
  while ( 1 ) {
    struct yr_message *command_message = NULL;
    bool ok = yr_recv_message(sock, &command_message, NULL);
    if ( !ok ) {
      errx(EX_SOFTWARE, "inferior exiting due to error");
    }
    ok = inferior_handle_message(sock, command_message, collection, runtime_callbacks, &state);
    if ( !ok ) {
      errx(EX_SOFTWARE, "failed to handle command");
    }
    free(command_message);
  }
}

#endif // YACHTROCK_POSIXY

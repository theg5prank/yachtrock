#if __APPLE__
// for u_int (for sysctl)
#define _DARWIN_C_SOURCE
#endif

#include <yachtrock/yachtrock.h>

#if YACHTROCK_MULTIPROCESS

#include <err.h>
#include <stdint.h>
#include <sysexits.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#if __APPLE__
#include <sys/sysctl.h>
#endif // __APPLE__

#include "multiprocess_inferior.h"
#include "yrutil.h"

#define HAVE_DEBUGGER_DETECTION __APPLE__ || __linux__

static bool is_debugger_attached(void)
{
  bool result = false;
#if __APPLE__
  int mib[4];
  struct kinfo_proc info;
  size_t size;

  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PID;
  mib[3] = getpid();

  size = sizeof(info);
  if ( sysctl(mib, sizeof(mib) / sizeof(mib[0]), &info, &size, NULL, 0) != 0 ) {
    yr_warn("sysctl in %s", __FUNCTION__);
  } else {
    result = (info.kp_proc.p_flag & P_TRACED);
  }
#elif __linux__
  /* Search for TracerPid in /proc/self/status */
  static const char *path = "/proc/self/status";
  FILE *f = fopen(path, "r");
  if ( !f ) {
    yr_warn("couldn't open %s", path);
    goto out;
  }
  char linebuf[512]; // more than enough for a valid TracerPid line
  bool found = false;
  while ( !found && !feof(f) && !ferror(f) ) {
    static const char * const tracerpid_needle = "TracerPid:";
    const char *tracerpid_loc = NULL;
    if ( fgets(linebuf, sizeof(linebuf), f) == NULL ) {
      if ( ferror(f) ) {
        yr_warn("fgets in %s", __FUNCTION__);
      }
    } else if ( (tracerpid_loc = strstr(linebuf, tracerpid_needle)) ) {
      found = true;
      result = strtol(tracerpid_loc + strlen(tracerpid_needle), NULL, 10) > 0;
    }
  }

 out:
  if ( f ) {
    fclose(f);
  }
#else
#if HAVE_DEBUGGER_DETECTION
#error "Should not reach here on a platform with debugger detection."
#endif
#endif
  return result;
}

static volatile sig_atomic_t sigcont_caught = 0;

static void sigcont_handler(int signo)
{
  sigcont_caught = 1;
}

static void wait_for_debugger(void)
{
  sigcont_caught = 0;

  struct sigaction new_action = { .sa_handler = 0 };
  struct sigaction old_action = { .sa_handler = 0 };
  sigemptyset(&new_action.sa_mask); // sure whatever.
  new_action.sa_handler = sigcont_handler;
  int result = 0;
  EINTR_RETRY(result = sigaction(SIGCONT, &new_action, &old_action));
  if ( result != 0 ) {
    yr_err(EX_OSERR, "sigaction failed: set in %s", __FUNCTION__);
  }

  sigset_t new_set;
  sigemptyset(&new_set);
  sigaddset(&new_set, SIGCONT);
  sigset_t old_set;
  EINTR_RETRY(result = sigprocmask(SIG_UNBLOCK, &new_set, &old_set));
  if ( result != 0 ) {
    yr_err(EX_OSERR, "sigprocmask failed: set in %s", __FUNCTION__);
  }

  while ( !sigcont_caught && !is_debugger_attached() ) {
    struct timespec waittime;
    waittime.tv_sec = 0;
    waittime.tv_nsec = 250000000;
    nanosleep(&waittime, NULL);
  };

  EINTR_RETRY(result = sigprocmask(SIG_SETMASK, &old_set, NULL));
  if ( result != 0 ) {
    yr_err(EX_OSERR, "sigprocmask failed: unwind in %s", __FUNCTION__);
  }

  EINTR_RETRY(result = sigaction(SIGCONT, &old_action, NULL));
  if ( result != 0 ) {
    yr_err(EX_OSERR, "sigaction failed: unwind in %s", __FUNCTION__);
  }
}

static void maybe_wait_for_debugger(void)
{
  if ( getenv(DEBUG_INFERIOR_VAR) ) {
    if ( unsetenv(DEBUG_INFERIOR_VAR) ) {
      yr_warn("unsetenv in %s", __FUNCTION__);
    }
    static const char * const msg_fmt =
#if HAVE_DEBUGGER_DETECTION
      "Waiting for debugger attach or SIGCONT to pid %d"
#else
      "Waiting for SIGCONT to pid %d"
#endif
      ;
    yr_warnx(msg_fmt, (int)getpid());
    wait_for_debugger();
  }
}

struct inferior_state {
  yr_test_suite_t opened_suite;
};

static bool send_collection_desc_message(int sock, yr_test_suite_collection_t collection)
{
  size_t collection_desc_size = yr_multiprocess_collection_desc(NULL, 0, collection);
  struct yr_message *message = yr_malloc(sizeof(struct yr_message) + collection_desc_size);
  size_t filled_out_size = yr_multiprocess_collection_desc(message->payload,
                                                           collection_desc_size,
                                                           collection);
  assert(filled_out_size == collection_desc_size);
  message->message_code = MESSAGE_PROVIDE_COLLECTION_DESC;
  message->payload_length = collection_desc_size;
  bool ok = yr_send_message(sock, message, NULL);
  if ( !ok ) {
    yr_warnx("failed to send collection desc message");
  }
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
  bool ok = yr_send_message(sock, update_message, NULL);
  free(update_message);
  return ok;
}

static void note_potential_new_result(struct inferior_runtime_context *context,
                                      yr_result_t potential_new_result)
{
  yr_result_t real_new_result = yr_merge_result(context->current_result, potential_new_result);
  if ( real_new_result != context->current_result ) {
    bool ok = send_result_update(context->sock, context->suite_index,
                                 context->case_index, real_new_result);
    if ( !ok ) {
      yr_errx(EX_SOFTWARE, "failure sending result update");
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
    yr_errx(EX_SOFTWARE, "bogus message payload length from superior");
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
  if ( !responded_ok ) {
    yr_warnx("unable to send case finished message");
  }
  free(case_finished_message);

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
    exit(0);
  case MESSAGE_INVOKE_CASE:
    return handle_invoke_case(sock, command_message, collection, runtime_callbacks, state);
    break;
  default:
    yr_warnx("unknown command %ld", (long)command_message->message_code);
    return false;
  }
}

void yr_inferior_loop(yr_test_suite_collection_t collection,
                      struct yr_runtime_callbacks runtime_callbacks)
{
  maybe_wait_for_debugger();
  struct inferior_state state;
  state.opened_suite = NULL;
  int sock = yr_inferior_socket();
  while ( 1 ) {
    struct yr_message *command_message = NULL;
    bool ok = yr_recv_message(sock, &command_message, NULL);
    if ( !ok ) {
      yr_errx(EX_SOFTWARE, "inferior exiting due to error");
    }
    ok = inferior_handle_message(sock, command_message, collection, runtime_callbacks, &state);
    if ( !ok ) {
      yr_errx(EX_SOFTWARE, "failed to handle command");
    }
    free(command_message);
  }
}

#endif // YACHTROCK_MULTIPROCESS

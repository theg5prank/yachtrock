#include <yachtrock/yachtrock.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

struct opened_testsuite
{
  struct yr_result_callbacks saved_result_callbacks;
  yr_test_suite_t suite;
  size_t num_cases;
  yr_test_case_s *cases;
  yr_result_store_t store;
  yr_result_store_t current_case_store;
};

static struct opened_testsuite *open_suite(yr_test_suite_t suite, struct yr_result_callbacks callbacks,
                                           struct yr_result_hooks result_hooks, void *result_refcon)
{
  struct opened_testsuite *collection = malloc(sizeof(struct opened_testsuite));
  collection->saved_result_callbacks = yr_set_result_callbacks(callbacks);
  collection->suite = suite;
  collection->num_cases = suite->num_cases;
  collection->cases = suite->cases;
  if ( suite->lifecycle.setup_suite ) {
    suite->lifecycle.setup_suite(suite);
  }
  collection->store = yr_result_store_create_with_hooks(suite->name, result_hooks, result_refcon);
  collection->current_case_store = NULL;
  return collection;
}

static void close_opened_suite(struct opened_testsuite *opened_suite)
{
  yr_result_store_close(opened_suite->store);
  if ( opened_suite->suite->lifecycle.teardown_suite ) {
    opened_suite->suite->lifecycle.teardown_suite(opened_suite->suite);
  }
  yr_set_result_callbacks(opened_suite->saved_result_callbacks);
}
static void destroy_opened_suite(struct opened_testsuite *opened_suite)
{
  yr_result_store_destroy(opened_suite->store);
  free(opened_suite);
}
static void execute_case(struct opened_testsuite *opened_suite, yr_test_case_t testcase)
{
  opened_suite->current_case_store = yr_result_store_open_subresult(opened_suite->store, testcase->name);
  if ( testcase->suite->lifecycle.setup_case ) {
    testcase->suite->lifecycle.setup_case(testcase);
  }
  testcase->testcase(testcase);
  if ( testcase->suite->lifecycle.teardown_case ) {
    testcase->suite->lifecycle.teardown_case(testcase);
  }
  yr_result_store_close(opened_suite->current_case_store);
  opened_suite->current_case_store = NULL;
}

static void basic_run_suite_note_assertion_failed(const char *assertion, const char *file,
                                                  size_t line, const char *funname,
                                                  const char *s, va_list ap, void *refcon)
{
  char _;
  va_list copy;
  va_copy(copy, ap);
  int necessary = vsnprintf(&_, 1, s, copy) + 1;
  va_end(copy);
  char desc[necessary];
  vsnprintf(desc, sizeof(desc), s, ap);
  fprintf(stderr, "Assertion \"%s\" failed: %s:%zu (in %s): %s\n", assertion, file, line, funname, desc);
}
static void basic_run_suite_note_skipped(const char *file, size_t line, const char *funname,
                                         const char *reason, va_list ap, void *refcon)
{
  char _;
  va_list copy;
  va_copy(copy, ap);
  int necessary = vsnprintf(&_, 1, reason, copy) + 1;
  va_end(copy);
  char desc[necessary];
  vsnprintf(desc, sizeof(desc), reason, ap);
  fprintf(stderr, "Skipping test (%s:%zu, in %s): %s\n", file, line, funname, desc);
}

struct basic_result_store_hooks_refcon
{
};
static void yr_basic_store_opened_callback(yr_result_store_t store, void *refcon)
{
  if ( yr_result_store_get_parent(store) == NULL ) {
    fprintf(stderr, "opening test suite %s\n", yr_result_store_get_name(store));
  } else {
    fprintf(stderr, "running test %s... ", yr_result_store_get_name(store));
  }
}
static void yr_basic_store_closed_callback(yr_result_store_t store, void *refcon)
{
  if ( yr_result_store_get_parent(store) == NULL ) {
    fprintf(stderr, "finished test suite %s\n", yr_result_store_get_name(store));
  } else {
    char *output = NULL;
    switch ( yr_result_store_get_result(store) ) {
    case YR_RESULT_UNSET:
      output = "[NO RESULT (?)]";
      break;
    case YR_RESULT_PASSED:
      output = "[OK]";
      break;
    case YR_RESULT_FAILED:
      output = "[FAIL]";
      break;
    case YR_RESULT_SKIPPED:
      output = "[SKIPPED]";
      break;
    }
    fprintf(stderr, "%s\n", output);
  }
}

int yr_basic_run_suite(yr_test_suite_t suite)
{
  struct yr_result_callbacks result_callbacks = {
    .note_assertion_failed = basic_run_suite_note_assertion_failed,
    .note_skipped = basic_run_suite_note_skipped,
    .refcon = NULL,
  };
  struct yr_result_hooks basic_hooks = {
    .store_opened = yr_basic_store_opened_callback,
    .store_closed = yr_basic_store_closed_callback,
  };
  struct basic_result_store_hooks_refcon store_hooks_context;
  return yr_run_suite_with_result_hooks(suite, basic_hooks, &store_hooks_context,
                                        result_callbacks);
}

struct run_suite_with_result_hooks_runtime_context
{
  struct opened_testsuite *suite;
  struct yr_result_callbacks provided_result_callbacks;
};

static void yr_run_suite_with_result_hooks_assertion_callback(const char *assertion, const char *file,
                                                              size_t line, const char *funname,
                                                              const char *s, va_list ap, void *refcon)
{
  struct run_suite_with_result_hooks_runtime_context *context = refcon;
  if ( context->provided_result_callbacks.note_assertion_failed ) {
    context->provided_result_callbacks.note_assertion_failed(assertion, file, line, funname, s, ap,
                                                             context->provided_result_callbacks.refcon);
  }
  yr_result_store_record_result(context->suite->current_case_store,
                                YR_RESULT_FAILED);
}
static void yr_run_suite_with_result_hooks_skipped_callback(const char *file, size_t line,
                                                            const char *funname, const char *reason,
                                                            va_list ap, void *refcon)
{
  struct run_suite_with_result_hooks_runtime_context *context = refcon;
  if ( context->provided_result_callbacks.note_skipped ) {
    context->provided_result_callbacks.note_skipped(file, line, funname, reason, ap,
                                                    context->provided_result_callbacks.refcon);
  }
  yr_result_store_record_result(context->suite->current_case_store, YR_RESULT_SKIPPED);
}

int yr_run_suite_with_result_hooks(yr_test_suite_t suite, struct yr_result_hooks hooks,
                                   void *result_hook_refcon,
                                   struct yr_result_callbacks provided_result_callbacks)
{
  struct yr_result_callbacks result_callbacks = {0};
  struct run_suite_with_result_hooks_runtime_context runtime_context;
  result_callbacks.refcon = &runtime_context;
  result_callbacks.note_assertion_failed = yr_run_suite_with_result_hooks_assertion_callback;
  result_callbacks.note_skipped = yr_run_suite_with_result_hooks_skipped_callback;
  struct opened_testsuite *opened = open_suite(suite, result_callbacks, hooks, result_hook_refcon);
  runtime_context.suite = opened;
  runtime_context.provided_result_callbacks = provided_result_callbacks;
  for ( size_t i = 0; i < opened->num_cases; i++ ) {
    execute_case(opened, &(opened->cases[i]));
  }
  close_opened_suite(opened);
  bool success = yr_result_store_get_result(opened->store) == YR_RESULT_PASSED;
  destroy_opened_suite(opened);
  return !success;
}

#include <yachtrock/yachtrock.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "yrutil.h"

struct opened_testsuite
{
  struct yr_runtime_callbacks saved_runtime_callbacks;
  yr_test_suite_t suite;
  size_t num_cases;
  yr_test_case_s *cases;
  yr_result_store_t store;
  yr_result_store_t current_case_store;
  bool owns_result_store;
};

static void _init_opened_suite_except_store(struct opened_testsuite *collection, yr_test_suite_t suite,
                                            struct yr_runtime_callbacks runtime_callbacks)
{
  collection->saved_runtime_callbacks = yr_set_runtime_callbacks(runtime_callbacks);
  collection->suite = suite;
  collection->num_cases = suite->num_cases;
  collection->cases = suite->cases;
  if ( suite->lifecycle.setup_suite ) {
    suite->lifecycle.setup_suite(suite);
  }
}
static struct opened_testsuite *open_suite(yr_test_suite_t suite,
                                           struct yr_runtime_callbacks runtime_callbacks,
                                           struct yr_result_hooks result_hooks)
{
  struct opened_testsuite *collection = yr_malloc(sizeof(struct opened_testsuite));
  _init_opened_suite_except_store(collection, suite, runtime_callbacks);
  collection->store = yr_result_store_create_with_hooks(suite->name, result_hooks);
  collection->owns_result_store = true;
  collection->current_case_store = NULL;
  return collection;
}
static struct opened_testsuite *open_suite_as_child_of_store(yr_test_suite_t suite,
                                                             struct yr_runtime_callbacks runtime_callbacks,
                                                             yr_result_store_t root)
{
  struct opened_testsuite *collection = yr_malloc(sizeof(struct opened_testsuite));
  _init_opened_suite_except_store(collection, suite, runtime_callbacks);
  collection->store = yr_result_store_open_subresult(root, suite->name);
  collection->owns_result_store = false;
  collection->current_case_store = NULL;
  return collection;
}
static void close_opened_suite(struct opened_testsuite *opened_suite)
{
  yr_result_store_close(opened_suite->store);
  if ( opened_suite->suite->lifecycle.teardown_suite ) {
    opened_suite->suite->lifecycle.teardown_suite(opened_suite->suite);
  }
  yr_set_runtime_callbacks(opened_suite->saved_runtime_callbacks);
}
static void destroy_opened_suite(struct opened_testsuite *opened_suite)
{
  if ( opened_suite->owns_result_store ) {
    yr_result_store_destroy(opened_suite->store);
  }
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

static int indentation_spaces(yr_result_store_t store)
{
  // ew?
  unsigned depth = 0;
  yr_result_store_t iter = store;
  do {
    iter = yr_result_store_get_parent(iter);
  } while ( iter && ++depth );
  return depth * 4;
}
static void yr_basic_store_opened_callback(yr_result_store_t store, void *refcon)
{
  fprintf(stderr, "%*sopening %s\n", indentation_spaces(store), "",
          yr_result_store_get_name(store));
}

static void yr_basic_store_closed_callback(yr_result_store_t store, void *refcon)
{
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

  fprintf(stderr, "%*sclosing %s %s\n", indentation_spaces(store), "",
          yr_result_store_get_name(store), output);
}

struct yr_runtime_callbacks YR_BASIC_STDERR_RUNTIME_CALLBACKS = {
  .note_assertion_failed = basic_run_suite_note_assertion_failed,
  .note_skipped = basic_run_suite_note_skipped,
  .refcon = NULL
};

struct yr_result_hooks YR_BASIC_STDERR_RESULT_HOOKS = {
  .store_opened = yr_basic_store_opened_callback,
  .store_closed = yr_basic_store_closed_callback,
  .context = NULL
};

bool yr_basic_run_suite(yr_test_suite_t suite)
{
  return yr_run_suite_with_result_hooks(suite, YR_BASIC_STDERR_RESULT_HOOKS,
                                        YR_BASIC_STDERR_RUNTIME_CALLBACKS);
}

struct run_suite_with_result_hooks_runtime_context
{
  struct opened_testsuite *suite;
  struct yr_runtime_callbacks provided_runtime_callbacks;
};

static void yr_run_suite_with_result_hooks_assertion_callback(const char *assertion, const char *file,
                                                              size_t line, const char *funname,
                                                              const char *s, va_list ap, void *refcon)
{
  struct run_suite_with_result_hooks_runtime_context *context = refcon;
  if ( context->provided_runtime_callbacks.note_assertion_failed ) {
    context->provided_runtime_callbacks.note_assertion_failed(assertion, file, line, funname, s, ap,
                                                              context->provided_runtime_callbacks.refcon);
  }
  yr_result_store_record_result(context->suite->current_case_store,
                                YR_RESULT_FAILED);
}
static void yr_run_suite_with_result_hooks_skipped_callback(const char *file, size_t line,
                                                            const char *funname, const char *reason,
                                                            va_list ap, void *refcon)
{
  struct run_suite_with_result_hooks_runtime_context *context = refcon;
  if ( context->provided_runtime_callbacks.note_skipped ) {
    context->provided_runtime_callbacks.note_skipped(file, line, funname, reason, ap,
                                                     context->provided_runtime_callbacks.refcon);
  }
  yr_result_store_record_result(context->suite->current_case_store, YR_RESULT_SKIPPED);
}

typedef struct opened_testsuite *(*testsuite_opener)(yr_test_suite_t suite,
                                                     struct yr_runtime_callbacks runtime_callbacks,
                                                     void *refcon);

static bool
_yr_run_suite_with_result_hooks_with_testsuite_opener(yr_test_suite_t suite,
                                                      struct yr_runtime_callbacks provided_runtime_callbacks,
                                                      void *opener_refcon,
                                                      testsuite_opener opener)
{
  struct yr_runtime_callbacks runtime_callbacks = {0};
  struct run_suite_with_result_hooks_runtime_context runtime_context;
  runtime_callbacks.refcon = &runtime_context;
  runtime_callbacks.note_assertion_failed = yr_run_suite_with_result_hooks_assertion_callback;
  runtime_callbacks.note_skipped = yr_run_suite_with_result_hooks_skipped_callback;
  struct opened_testsuite *opened = opener(suite, runtime_callbacks, opener_refcon);
  runtime_context.suite = opened;
  runtime_context.provided_runtime_callbacks = provided_runtime_callbacks;
  for ( size_t i = 0; i < opened->num_cases; i++ ) {
    execute_case(opened, &(opened->cases[i]));
  }
  close_opened_suite(opened);
  bool success = yr_result_store_get_result(opened->store) == YR_RESULT_PASSED;
  destroy_opened_suite(opened);
  return success;
}

struct yr_run_suite_with_result_hooks_opener_context
{
  struct yr_result_hooks hooks;
};
static struct opened_testsuite *
_yr_run_suite_with_result_hooks_opener(yr_test_suite_t suite,
                                       struct yr_runtime_callbacks runtime_callbacks,
                                       void *refcon)
{
  struct yr_run_suite_with_result_hooks_opener_context *context = refcon;
  return open_suite(suite, runtime_callbacks, context->hooks);
}
bool yr_run_suite_with_result_hooks(yr_test_suite_t suite, struct yr_result_hooks hooks,
                                    struct yr_runtime_callbacks provided_runtime_callbacks)
{
  struct yr_run_suite_with_result_hooks_opener_context opener_context;
  opener_context.hooks = hooks;
  return _yr_run_suite_with_result_hooks_with_testsuite_opener(suite,
                                                               provided_runtime_callbacks,
                                                               &opener_context,
                                                               _yr_run_suite_with_result_hooks_opener);
}

struct yr_run_suite_under_store_opener_context
{
  yr_result_store_t store;
};
static struct opened_testsuite *
_yr_run_suite_under_store_opener(yr_test_suite_t suite,
                                 struct yr_runtime_callbacks runtime_callbacks,
                                 void *refcon)
{
  yr_result_store_t store = ((struct yr_run_suite_under_store_opener_context *)refcon)->store;
  return open_suite_as_child_of_store(suite, runtime_callbacks, store);
}
void yr_run_suite_under_store(yr_test_suite_t suite,
                              yr_result_store_t store,
                              struct yr_runtime_callbacks provided_runtime_callbacks)
{
  struct yr_run_suite_under_store_opener_context opener_context;
  opener_context.store = store;
  _yr_run_suite_with_result_hooks_with_testsuite_opener(suite,
                                                        provided_runtime_callbacks,
                                                        &opener_context,
                                                        _yr_run_suite_under_store_opener);
}

void yr_run_suite_collection_under_store(yr_test_suite_collection_t collection,
                                         yr_result_store_t store,
                                         struct yr_runtime_callbacks runtime_callbacks)
{
  for ( size_t i = 0; i < collection->num_suites; i++ ) {
    yr_run_suite_under_store(collection->suites[i], store, runtime_callbacks);
  }
}

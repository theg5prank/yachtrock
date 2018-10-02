#include <yachtrock/yachtrock.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

struct opened_testsuite
{
  struct result_callbacks saved_result_callbacks;
  yr_test_suite_t suite;
  size_t num_cases;
  yr_test_case_s *cases;
};

struct opened_testsuite open_suite(yr_test_suite_t suite, struct result_callbacks callbacks)
{
  struct opened_testsuite collection;
  collection.saved_result_callbacks = yr_set_result_callbacks(callbacks);
  collection.suite = suite;
  collection.num_cases = suite->num_cases;
  collection.cases = suite->cases;
  if ( suite->lifecycle.setup_suite ) {
    suite->lifecycle.setup_suite(suite);
  }
  return collection;
}

void close_opened_suite(struct opened_testsuite opened_suite)
{
  if ( opened_suite.suite->lifecycle.teardown_suite ) {
    opened_suite.suite->lifecycle.teardown_suite(opened_suite.suite);
  }
  yr_set_result_callbacks(opened_suite.saved_result_callbacks);
}

void execute_case(yr_test_case_s testcase)
{
  if ( testcase.suite->lifecycle.setup_case ) {
    testcase.suite->lifecycle.setup_case(testcase);
  }
  testcase.testcase(testcase);
  if ( testcase.suite->lifecycle.teardown_case ) {
    testcase.suite->lifecycle.teardown_case(testcase);
  }
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
  *(bool *)refcon = false;
}
int yr_basic_run_suite(yr_test_suite_t suite)
{
  bool suite_ok = true;
  bool test_ok = true;
  fprintf(stderr, "opening test suite %s\n", suite->name);
  struct result_callbacks callbacks = {0};
  callbacks.refcon = &test_ok;
  callbacks.note_assertion_failed = basic_run_suite_note_assertion_failed;
  struct opened_testsuite opened = open_suite(suite, callbacks);
  for ( size_t i = 0; i < opened.num_cases; i++ ) {
    yr_test_case_s testcase = opened.cases[i];
    fprintf(stderr, "running test %s... ", testcase.name);
    execute_case(testcase);
    fprintf(stderr, "%s\n", test_ok ? "[OK]" : "[FAIL]");
    suite_ok = suite_ok && test_ok;
    test_ok = true;
  }
  fprintf(stderr, "finished test suite %s\n", suite->name);
  close_opened_suite(opened);
  return suite_ok ? 0 : 1;
}

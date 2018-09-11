#include <yachtrock/yachtrock.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

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
  struct result_callbacks old = yr_set_result_callbacks(callbacks);
  for ( size_t i = 0; i < suite->num_cases; i++ ) {
    yr_test_case_s testcase = suite->cases[i];
    fprintf(stderr, "running test %s... ", testcase.name);
    testcase.testcase(testcase);
    fprintf(stderr, "%s\n", test_ok ? "[OK]" : "[FAIL]");
    suite_ok = suite_ok && test_ok;
    test_ok = true;
  }
  fprintf(stderr, "finished test suite %s\n", suite->name);
  yr_set_result_callbacks(old);
  return suite_ok ? 0 : 1;
}

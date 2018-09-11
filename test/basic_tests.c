#include <yachtrock/yachtrock.h>
#include <stdlib.h>
#include <stdio.h>

void test_basic_assert_failure(yr_test_case_s testcase)
{
  YR_ASSERT(1 == 2, "expected one to equal two, but it was: %d", 1);
}

void test_basic_assert_passes_refcon(yr_test_case_s testcase)
{
  YR_ASSERT(*(int *)testcase.refcon = 1, "test refcon should be set");
  YR_ASSERT(*(int *)testcase.suite->refcon = 2, "suite refcon should be set");
}

int main(void)
{
  yr_test_suite_t suite = calloc(sizeof(yr_test_suite_s) + sizeof(yr_test_case_s) * 1, 1);
  suite->name = "Basic test";
  suite->refcon = NULL;
  suite->num_cases = 1;
  suite->cases[0].name = "test_basic_assert_failure";
  suite->cases[0].refcon = NULL;
  suite->cases[0].testcase = test_basic_assert_failure;
  suite->cases[0].suite = suite;

  if ( yr_basic_run_suite(suite) != 1 ) {
    fprintf(stderr, "test unexpectedly passed!\n");
    abort();
  }

  suite->name = "Test of refcon";
  int one = 1, two = 2;
  suite->refcon = &one;
  suite->cases[0].name = "test_basic_assert_passes_refcon";
  suite->cases[0].refcon = &two;
  suite->cases[0].testcase = test_basic_assert_passes_refcon;
  if ( yr_basic_run_suite(suite) == 1 ) {
    fprintf(stderr, "test unexpectedly failed!\n");
    abort();
  }
  return 0;
}

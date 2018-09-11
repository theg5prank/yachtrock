#include <yachtrock/yachtrock.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

void test_basic_assert_failure(yr_test_case_s testcase)
{
  YR_ASSERT(1 == 2, "expected one to equal two, but it was: %d (pssst: this is a dummy failure)", 1);
}

void test_basic_assert_passes_refcon(yr_test_case_s testcase)
{
  YR_ASSERT(*(int *)testcase.suite->refcon == 2, "suite refcon should be set");
}

struct setup_teardown_test_data
{
  unsigned case_setups_run;
  unsigned case_teardowns_run;
  unsigned suite_setups_run;
  unsigned suite_teardowns_run;
  unsigned num_cases_run;
};
void setup_test_suite_setup(yr_test_suite_t suite)
{
  ((struct setup_teardown_test_data *)suite->refcon)->suite_setups_run++;
}
void setup_test_suite_teardown(yr_test_suite_t suite)
{
  ((struct setup_teardown_test_data *)suite->refcon)->suite_teardowns_run++;
}
void setup_test_case_setup(yr_test_case_s testcase)
{
  ((struct setup_teardown_test_data *)testcase.suite->refcon)->case_setups_run++;
}
void setup_test_case_teardown(yr_test_case_s testcase)
{
  ((struct setup_teardown_test_data *)testcase.suite->refcon)->case_teardowns_run++;
}
void test_suite_and_case_setups_run_pt1(yr_test_case_s testcase)
{
  YR_ASSERT(((struct setup_teardown_test_data *)testcase.suite->refcon)->case_setups_run == 1,
            "case setup should be run one time now");
  YR_ASSERT(((struct setup_teardown_test_data *)testcase.suite->refcon)->case_teardowns_run == 0,
            "case teardown should be run zero times now");
  YR_ASSERT(((struct setup_teardown_test_data *)testcase.suite->refcon)->suite_setups_run == 1,
            "suite setup should be run one time now");
  YR_ASSERT(((struct setup_teardown_test_data *)testcase.suite->refcon)->suite_teardowns_run == 0,
            "suite teardown should be run zero times now");
  ((struct setup_teardown_test_data *)testcase.suite->refcon)->num_cases_run++;
}
void test_suite_and_case_setups_run_pt2(yr_test_case_s testcase)
{
  YR_ASSERT(((struct setup_teardown_test_data *)testcase.suite->refcon)->case_setups_run == 2,
            "case setup should be run one time now");
  YR_ASSERT(((struct setup_teardown_test_data *)testcase.suite->refcon)->case_teardowns_run == 1,
            "case teardown should be run zero times now");
  YR_ASSERT(((struct setup_teardown_test_data *)testcase.suite->refcon)->suite_setups_run == 1,
            "suite setup should be run one time now");
  YR_ASSERT(((struct setup_teardown_test_data *)testcase.suite->refcon)->suite_teardowns_run == 0,
            "suite teardown should be run zero times now");
  ((struct setup_teardown_test_data *)testcase.suite->refcon)->num_cases_run++;
}

void do_test_basics_setups_teardowns(void)
{
  struct setup_teardown_test_data data;
  memset(&data, 0, sizeof(data));
  yr_test_suite_t suite = calloc(sizeof(yr_test_suite_s) + sizeof(yr_test_case_s) * 2, 1);
  suite->name = "Basic setup/teardown test";
  suite->refcon = &data;
  suite->num_cases = 2;
  suite->setup_case = setup_test_case_setup;
  suite->teardown_case = setup_test_case_teardown;
  suite->setup_suite = setup_test_suite_setup;
  suite->teardown_suite = setup_test_suite_teardown;
  suite->cases[0].name = "test_suite_and_case_setups_run_pt1";
  suite->cases[0].testcase = test_suite_and_case_setups_run_pt1;
  suite->cases[0].suite = suite;
  suite->cases[1].name = "test_suite_and_case_setups_run_pt2";
  suite->cases[1].testcase = test_suite_and_case_setups_run_pt2;
  suite->cases[1].suite = suite;
  yr_basic_run_suite(suite);
  assert(data.case_setups_run == 2);
  assert(data.case_teardowns_run == 2);
  assert(data.suite_setups_run == 1);
  assert(data.suite_teardowns_run == 1);
  assert(data.num_cases_run == 2);
}

int main(void)
{
  yr_test_suite_t suite = calloc(sizeof(yr_test_suite_s) + sizeof(yr_test_case_s) * 1, 1);
  suite->name = "Basic test";
  suite->refcon = NULL;
  suite->num_cases = 1;
  suite->cases[0].name = "test_basic_assert_failure";
  suite->cases[0].testcase = test_basic_assert_failure;
  suite->cases[0].suite = suite;

  if ( yr_basic_run_suite(suite) != 1 ) {
    fprintf(stderr, "test unexpectedly passed!\n");
    abort();
  }

  suite->name = "Test of refcon";
  int two = 2;
  suite->refcon = &two;
  suite->cases[0].name = "test_basic_assert_passes_refcon";
  suite->cases[0].testcase = test_basic_assert_passes_refcon;
  if ( yr_basic_run_suite(suite) == 1 ) {
    fprintf(stderr, "test unexpectedly failed!\n");
    abort();
  }

  do_test_basics_setups_teardowns();
  return 0;
}

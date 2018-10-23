#include <yachtrock/yachtrock.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

void test_basic_assert_failure(yr_test_case_t testcase)
{
  YR_ASSERT(1 == 2, "expected one to equal two, but it was: %d (pssst: this is a dummy failure)", 1);
}

void test_basic_assert_passes_refcon(yr_test_case_t testcase)
{
  YR_ASSERT(*(int *)testcase->suite->refcon == 2, "suite refcon should be set");
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
void setup_test_case_setup(yr_test_case_t testcase)
{
  ((struct setup_teardown_test_data *)testcase->suite->refcon)->case_setups_run++;
}
void setup_test_case_teardown(yr_test_case_t testcase)
{
  ((struct setup_teardown_test_data *)testcase->suite->refcon)->case_teardowns_run++;
}
void test_suite_and_case_setups_run_pt1(yr_test_case_t testcase)
{
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->case_setups_run == 1,
            "case setup should be run one time now");
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->case_teardowns_run == 0,
            "case teardown should be run zero times now");
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->suite_setups_run == 1,
            "suite setup should be run one time now");
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->suite_teardowns_run == 0,
            "suite teardown should be run zero times now");
  ((struct setup_teardown_test_data *)testcase->suite->refcon)->num_cases_run++;
}
void test_suite_and_case_setups_run_pt2(yr_test_case_t testcase)
{
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->case_setups_run == 2,
            "case setup should be run one time now");
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->case_teardowns_run == 1,
            "case teardown should be run zero times now");
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->suite_setups_run == 1,
            "suite setup should be run one time now");
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->suite_teardowns_run == 0,
            "suite teardown should be run zero times now");
  ((struct setup_teardown_test_data *)testcase->suite->refcon)->num_cases_run++;
}

void do_test_basics_setups_teardowns(yr_test_case_t tc)
{
  struct setup_teardown_test_data data;
  memset(&data, 0, sizeof(data));
  yr_test_suite_t suite = calloc(sizeof(yr_test_suite_s) + sizeof(yr_test_case_s) * 2, 1);
  suite->name = "Basic setup/teardown test";
  suite->refcon = &data;
  suite->num_cases = 2;
  suite->lifecycle.setup_case = setup_test_case_setup;
  suite->lifecycle.teardown_case = setup_test_case_teardown;
  suite->lifecycle.setup_suite = setup_test_suite_setup;
  suite->lifecycle.teardown_suite = setup_test_suite_teardown;
  suite->cases[0].name = "test_suite_and_case_setups_run_pt1";
  suite->cases[0].testcase = test_suite_and_case_setups_run_pt1;
  suite->cases[0].suite = suite;
  suite->cases[1].name = "test_suite_and_case_setups_run_pt2";
  suite->cases[1].testcase = test_suite_and_case_setups_run_pt2;
  suite->cases[1].suite = suite;
  yr_basic_run_suite(suite);
  YR_ASSERT(data.case_setups_run == 2, "case setups run wrong");
  YR_ASSERT(data.case_teardowns_run == 2, "case teardowns run wrong");
  YR_ASSERT(data.suite_setups_run == 1, "suite setups run wrong");
  YR_ASSERT(data.suite_teardowns_run == 1, "suite teardowns run wrong");
  YR_ASSERT(data.num_cases_run == 2, "num cases run wrong");
  free(suite);
}

void do_test_basics_suite_from_functions(yr_test_case_t tc)
{
  struct setup_teardown_test_data data;
  memset(&data, 0, sizeof(data));
  struct yr_suite_lifecycle_callbacks callbacks;

  callbacks.setup_case = setup_test_case_setup;
  callbacks.teardown_case = setup_test_case_teardown;
  callbacks.setup_suite = setup_test_suite_setup;
  callbacks.teardown_suite = setup_test_suite_teardown;
  
  yr_test_suite_t suite = yr_create_suite_from_functions(__FUNCTION__, NULL, callbacks,
                                                         test_suite_and_case_setups_run_pt1, test_suite_and_case_setups_run_pt2);
  suite->refcon = &data;
  yr_basic_run_suite(suite);
  YR_ASSERT(data.case_setups_run == 2, "case setups run wrong");
  YR_ASSERT(data.case_teardowns_run == 2, "case teardowns run wrong");
  YR_ASSERT(data.suite_setups_run == 1, "suite setups run wrong");
  YR_ASSERT(data.suite_teardowns_run == 1, "suite teardowns run wrong");
  YR_ASSERT(data.num_cases_run == 2, "num cases run wrong");

  free(suite);

  suite = yr_create_suite_from_functions(__FUNCTION__, NULL, callbacks,
                                         test_suite_and_case_setups_run_pt1);
  memset(&data, 0, sizeof(data));
  suite->refcon = &data;
  yr_basic_run_suite(suite);
  YR_ASSERT(data.case_setups_run == 1, "case setups run wrong");
  YR_ASSERT(data.case_teardowns_run == 1, "case teardowns run wrong");
  YR_ASSERT(data.suite_setups_run == 1, "suite setups run wrong");
  YR_ASSERT(data.suite_teardowns_run == 1, "suite teardowns run wrong");
  YR_ASSERT(data.num_cases_run == 1, "num cases run wrong");

  free(suite);
}

void do_test_failures(yr_test_case_t tc)
{
  yr_test_suite_t suite = yr_create_blank_suite(1);
  suite->name = "Basic test";
  suite->refcon = NULL;
  suite->cases[0].name = "test_basic_assert_failure";
  suite->cases[0].testcase = test_basic_assert_failure;
  YR_ASSERT(yr_basic_run_suite(suite) == 1, "test should have failed!");
  free(suite);
}

void do_test_refcon(yr_test_case_t tc)
{
  yr_test_suite_t suite = yr_create_blank_suite(1);
  suite->name = "Test of refcon";
  int two = 2;
  suite->refcon = &two;
  suite->cases[0].name = "test_basic_assert_passes_refcon";
  suite->cases[0].testcase = test_basic_assert_passes_refcon;
  YR_ASSERT(yr_basic_run_suite(suite) == 0, "test should have passed!");
  free(suite);
}

void test_skip_dummy(yr_test_case_t tc)
{
  YR_SKIP_AND_RETURN("forgot to frob the widget.");
  abort();
}
void do_test_skip_basics(yr_test_case_t tc)
{
  yr_test_suite_t suite = yr_create_suite_from_functions("skip basics subtest", NULL, YR_NO_CALLBACKS,
                                                         test_skip_dummy);
  YR_ASSERT(yr_basic_run_suite(suite) == 0);
  free(suite);
}

YR_TESTCASE(test_case_decl_name_only)
{
  YR_ASSERT(strlen(testcase->name) > 0);
}

YR_TESTCASE(test_case_decl_with_name, tc)
{
  YR_ASSERT(strlen(tc->name) > 0);
}

int main(void)
{
  yr_test_suite_t suite = yr_create_suite_from_functions("basic tests", NULL, YR_NO_CALLBACKS,
                                                         do_test_failures, do_test_refcon,
                                                         do_test_basics_setups_teardowns,
                                                         do_test_basics_suite_from_functions,
                                                         do_test_skip_basics,
                                                         test_case_decl_name_only,
                                                         test_case_decl_with_name);
  if ( yr_basic_run_suite(suite) ) {
    fprintf(stderr, "some tests failed!\n");
    return EXIT_FAILURE;
  }
  free(suite);
  return 0;
}

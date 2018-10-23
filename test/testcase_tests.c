#include <yachtrock/yachtrock.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void dummy1(yr_test_case_t tc) {}
void dummy2(yr_test_case_t tc) {}
void dummy3(yr_test_case_t tc) {}
void dummy_setup_case(yr_test_case_t tc) {}
void dummy_teardown_case(yr_test_case_t tc) {}
void dummy_setup_suite(yr_test_suite_t tc) {}
void dummy_teardown_suite(yr_test_suite_t tc) {}

YR_TESTCASE(test_create_from_functions)
{
  struct yr_suite_lifecycle_callbacks callbacks = {
    .setup_case = dummy_setup_case,
    .teardown_case = dummy_teardown_case,
    .setup_suite = dummy_setup_suite,
    .teardown_suite = dummy_teardown_suite,
  };

  yr_test_suite_t suite = yr_create_suite_from_functions("the name",
                                                         &callbacks,
                                                         dummy1, dummy2, dummy3);

  YR_ASSERT_EQUAL(strcmp(suite->name, "the name"), 0);
  YR_ASSERT_EQUAL(suite->num_cases, 3);
  YR_ASSERT_EQUAL(suite->lifecycle.setup_case, dummy_setup_case);
  YR_ASSERT_EQUAL(suite->lifecycle.teardown_case, dummy_teardown_case);
  YR_ASSERT_EQUAL(suite->lifecycle.setup_suite, dummy_setup_suite);
  YR_ASSERT_EQUAL(suite->lifecycle.teardown_suite, dummy_teardown_suite);

  YR_ASSERT_EQUAL(strcmp(suite->cases[0].name, "dummy1"), 0);
  YR_ASSERT_EQUAL(suite->cases[0].testcase, dummy1);
  YR_ASSERT_EQUAL(suite->cases[0].suite, suite);

  YR_ASSERT_EQUAL(strcmp(suite->cases[1].name, "dummy2"), 0);
  YR_ASSERT_EQUAL(suite->cases[1].testcase, dummy2);
  YR_ASSERT_EQUAL(suite->cases[1].suite, suite);

  YR_ASSERT_EQUAL(strcmp(suite->cases[2].name, "dummy3"), 0);
  YR_ASSERT_EQUAL(suite->cases[2].testcase, dummy3);
  YR_ASSERT_EQUAL(suite->cases[2].suite, suite);

  free(suite);
}

static void assert_suites_equal_but_independent(yr_test_suite_t a, yr_test_suite_t b)
{
  YR_ASSERT_NOT_EQUAL(a, b);

  YR_ASSERT_NOT_EQUAL(a->name, b->name);
  YR_ASSERT_EQUAL(strcmp(a->name, b->name), 0);

  YR_ASSERT_EQUAL(a->refcon, b->refcon);

  YR_ASSERT_EQUAL(a->lifecycle.setup_case, b->lifecycle.setup_case);
  YR_ASSERT_EQUAL(a->lifecycle.teardown_case, b->lifecycle.teardown_case);
  YR_ASSERT_EQUAL(a->lifecycle.setup_suite, b->lifecycle.setup_suite);
  YR_ASSERT_EQUAL(a->lifecycle.teardown_suite, b->lifecycle.teardown_suite);

  YR_ASSERT_EQUAL(a->num_cases, b->num_cases);
  for ( size_t i = 0; i < a->num_cases; i++ ) {
    YR_ASSERT_NOT_EQUAL(a->cases[i].name, b->cases[i].name);
    YR_ASSERT_EQUAL(strcmp(a->cases[i].name, b->cases[i].name), 0);
    YR_ASSERT_EQUAL(a->cases[i].testcase, b->cases[i].testcase);
    YR_ASSERT_EQUAL(a->cases[i].suite, a);
    YR_ASSERT_EQUAL(b->cases[i].suite, b);
  }
}

YR_TESTCASE(test_collection_from_suites_basic)
{
  struct yr_suite_lifecycle_callbacks callbacks = {
    .setup_case = dummy_setup_case,
    .teardown_case = dummy_teardown_case,
    .setup_suite = dummy_setup_suite,
    .teardown_suite = dummy_teardown_suite,
  };

  yr_test_suite_t suite = yr_create_suite_from_functions("the name",
                                                         &callbacks,
                                                         dummy1, dummy2, dummy3);

  yr_test_suite_t suites[] = {suite, suite};

  yr_test_suite_collection_t collection = yr_test_suite_collection_create_from_suites(2, suites);

  YR_ASSERT_EQUAL(collection->num_suites, 2);
  assert_suites_equal_but_independent(collection->suites[0], suite);
  assert_suites_equal_but_independent(collection->suites[1], suite);
  assert_suites_equal_but_independent(collection->suites[0], collection->suites[1]);

  free(suite);
  free(collection);
}

void dummy12(yr_test_case_t tc) {}
void dummy22(yr_test_case_t tc) {}
void dummy32(yr_test_case_t tc) {}
void dummy_setup_case2(yr_test_case_t tc) {}
void dummy_teardown_case2(yr_test_case_t tc) {}
void dummy_setup_suite2(yr_test_suite_t tc) {}
void dummy_teardown_suite2(yr_test_suite_t tc) {}


YR_TESTCASE(test_collection_from_suites_more)
{
  struct yr_suite_lifecycle_callbacks callbacks1 = {
    .setup_case = dummy_setup_case,
    .teardown_case = dummy_teardown_case,
    .setup_suite = dummy_setup_suite,
    .teardown_suite = dummy_teardown_suite,
  };

  struct yr_suite_lifecycle_callbacks callbacks2 = {
    .setup_case = dummy_setup_case2,
    .teardown_case = dummy_teardown_case2,
    .setup_suite = dummy_setup_suite2,
    .teardown_suite = dummy_teardown_suite2,
  };

  struct yr_suite_lifecycle_callbacks callbacks3 = {
    .setup_case = dummy_setup_case,
    .teardown_case = dummy_teardown_case2,
    .setup_suite = dummy_setup_suite2,
    .teardown_suite = dummy_teardown_suite,
  };

  yr_test_suite_t suite1 = yr_create_suite_from_functions("suite1",
                                                          &callbacks1,
                                                          dummy1, dummy2, dummy3);
  yr_test_suite_t suite2 = yr_create_suite_from_functions("suite2",
                                                          &callbacks2,
                                                          dummy12, dummy22, dummy32);
  yr_test_suite_t suite3 = yr_create_suite_from_functions("suite3",
                                                          &callbacks3,
                                                          dummy12, dummy2);
  yr_test_suite_t suite4 = yr_create_suite_from_functions("suite4",
                                                          NULL,
                                                          dummy1, dummy22, dummy32, dummy12);
  yr_test_suite_t suites[] = { suite1, suite2, suite3, suite4 };

  yr_test_suite_collection_t collection = yr_test_suite_collection_create_from_suites(4, suites);
  YR_ASSERT_EQUAL(collection->num_suites, 4);
  for ( size_t i = 0; i < 4; i++ ) {
    assert_suites_equal_but_independent(suites[i], collection->suites[i]);
    free(suites[i]);
  }
  free(collection);
}

int main(void)
{
  yr_test_suite_t suite = yr_create_suite_from_functions("testcase tests", NULL,
                                                         test_create_from_functions,
                                                         test_collection_from_suites_basic,
                                                         test_collection_from_suites_more);
  if ( yr_basic_run_suite(suite) ) {
    fprintf(stderr, "some tests failed!\n");
    return EXIT_FAILURE;
  }
  free(suite);
  return 0;

}

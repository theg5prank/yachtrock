#include <yachtrock/yachtrock.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static const char *env_key = "COLLECTION_CREATOR";

struct proc_info
{
  int argc;
  char **argv;
};

struct proc_info proc_info;

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
            "case setup should be run two times now");
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->case_teardowns_run == 1,
            "case teardown should be run one time now");
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->suite_setups_run == 1,
            "suite setup should be run one time now");
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->suite_teardowns_run == 0,
            "suite teardown should be run zero times now");
  ((struct setup_teardown_test_data *)testcase->suite->refcon)->num_cases_run++;
}
void test_suite_and_case_setups_run_pt3(yr_test_case_t testcase)
{
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->case_setups_run == 3,
            "case setup should be run one time now");
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->case_teardowns_run == 2,
            "case teardown should be run zero times now");
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->suite_setups_run == 2,
            "suite setup should be run twice now");
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->suite_teardowns_run == 1,
            "suite teardown should be run once now");
  YR_FAIL("unconditional fail");
}

yr_test_suite_collection_t create_setups_teardowns_suite_collection(struct setup_teardown_test_data **refcon)
{
  struct setup_teardown_test_data *data = malloc(sizeof(struct setup_teardown_test_data));
  memset(data, 0, sizeof(*data));
  yr_test_suite_t suite1 = calloc(sizeof(yr_test_suite_s) + sizeof(yr_test_case_s) * 2, 1);
  suite1->name = "Basic setup/teardown test";
  suite1->refcon = data;
  suite1->num_cases = 2;
  suite1->lifecycle.setup_case = setup_test_case_setup;
  suite1->lifecycle.teardown_case = setup_test_case_teardown;
  suite1->lifecycle.setup_suite = setup_test_suite_setup;
  suite1->lifecycle.teardown_suite = setup_test_suite_teardown;
  suite1->cases[0].name = "test_suite_and_case_setups_run_pt1";
  suite1->cases[0].testcase = test_suite_and_case_setups_run_pt1;
  suite1->cases[0].suite = suite1;
  suite1->cases[1].name = "test_suite_and_case_setups_run_pt2";
  suite1->cases[1].testcase = test_suite_and_case_setups_run_pt2;
  suite1->cases[1].suite = suite1;

  yr_test_suite_t suite2 = calloc(sizeof(yr_test_suite_s) + sizeof(yr_test_case_s), 1);
  suite2->name = "basic setup/teardown test";
  suite2->refcon = data;
  suite2->num_cases = 1;
  suite2->lifecycle.setup_case = setup_test_case_setup;
  suite2->lifecycle.teardown_case = setup_test_case_teardown;
  suite2->lifecycle.setup_suite = setup_test_suite_setup;
  suite2->lifecycle.teardown_suite = setup_test_suite_teardown;
  suite2->cases[0].name = "test_suite_and_case_setups_run_pt3";
  suite2->cases[0].testcase = test_suite_and_case_setups_run_pt3;
  suite2->cases[0].suite = suite2;

  yr_test_suite_t suites[] = {suite1, suite2};

  yr_test_suite_collection_t collection = yr_test_suite_collection_create_from_suites(2, suites);
  free(suite1);
  free(suite2);
  *refcon = data;
  return collection;
}

YR_TESTCASE(test_aborting_pt1)
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

YR_TESTCASE(test_aborting_pt2)
{
  abort();
}

YR_TESTCASE(test_aborting_pt3)
{
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->case_setups_run == 1);
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->case_teardowns_run == 0);
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->suite_setups_run == 1);
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->suite_teardowns_run == 0);
  ((struct setup_teardown_test_data *)testcase->suite->refcon)->num_cases_run++;
}

YR_TESTCASE(test_aborting_pt4)
{
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->case_setups_run == 2);
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->case_teardowns_run == 1);
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->suite_setups_run == 1);
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->suite_teardowns_run == 0);
  ((struct setup_teardown_test_data *)testcase->suite->refcon)->num_cases_run++;
}

YR_TESTCASE(test_aborting_pt5)
{
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->case_setups_run == 3);
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->case_teardowns_run == 2);
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->suite_setups_run == 2);
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->suite_teardowns_run == 1);
  ((struct setup_teardown_test_data *)testcase->suite->refcon)->num_cases_run++;
}

yr_test_suite_collection_t create_setups_teardowns_aborts_suite_collection(struct setup_teardown_test_data **refcon)
{
  struct setup_teardown_test_data *data = malloc(sizeof(struct setup_teardown_test_data));
  memset(data, 0, sizeof(*data));

  yr_test_suite_t suite1 = calloc(sizeof(yr_test_suite_s) + sizeof(yr_test_case_s) * 4, 1);
  suite1->name = "aborting test suite1";
  suite1->refcon = data;
  suite1->num_cases = 4;
  suite1->lifecycle.setup_case = setup_test_case_setup;
  suite1->lifecycle.teardown_case = setup_test_case_teardown;
  suite1->lifecycle.setup_suite = setup_test_suite_setup;
  suite1->lifecycle.teardown_suite = setup_test_suite_teardown;
  suite1->cases[0].name = "test_aborting_pt1";
  suite1->cases[0].testcase = test_aborting_pt1;
  suite1->cases[0].suite = suite1;
  suite1->cases[1].name = "test_aborting_pt2";
  suite1->cases[1].testcase = test_aborting_pt2;
  suite1->cases[1].suite = suite1;
  suite1->cases[2].name = "test_aborting_pt3";
  suite1->cases[2].testcase = test_aborting_pt3;
  suite1->cases[2].suite = suite1;
  suite1->cases[3].name = "test_aborting_pt4";
  suite1->cases[3].testcase = test_aborting_pt4;
  suite1->cases[3].suite = suite1;

  yr_test_suite_t suite2 = calloc(sizeof(yr_test_suite_s) + sizeof(yr_test_case_s), 1);
  suite2->name = "aborting test suite2";
  suite2->refcon = data;
  suite2->num_cases = 1;
  suite2->lifecycle.setup_case = setup_test_case_setup;
  suite2->lifecycle.teardown_case = setup_test_case_teardown;
  suite2->lifecycle.setup_suite = setup_test_suite_setup;
  suite2->lifecycle.teardown_suite = setup_test_suite_teardown;
  suite2->cases[0].name = "test_aborting_pt5";
  suite2->cases[0].testcase = test_aborting_pt5;
  suite2->cases[0].suite = suite2;

  yr_test_suite_t suites[] = {suite1, suite2};

  yr_test_suite_collection_t collection = yr_test_suite_collection_create_from_suites(2, suites);
  free(suite1);
  free(suite2);
  *refcon = data;
  return collection;
}

YR_TESTCASE(dummy1) { }
YR_TESTCASE(dummy2)
{
  YR_RECORD_SKIP("because I said so");
}

yr_test_suite_collection_t create_test_suspension_final_suite_collection(struct setup_teardown_test_data **refcon)
{
  yr_test_suite_t suite1 = yr_create_suite_from_functions("suspension final suite", NULL, YR_NO_CALLBACKS,
                                                          dummy1, dummy2);
  yr_test_suite_collection_t collection = yr_test_suite_collection_create_from_suites(1, &suite1);
  free(suite1);
  return collection;
}

static void intermediate(yr_test_case_t testcase);

yr_test_suite_collection_t create_test_suspension_suite_collection(struct setup_teardown_test_data **refcon)
{
  yr_test_suite_t suite1 = yr_create_suite_from_functions("intermediate suite", NULL, YR_NO_CALLBACKS,
                                                          intermediate);
  yr_test_suite_collection_t collection = yr_test_suite_collection_create_from_suites(1, &suite1);
  free(suite1);
  return collection;
}

static const struct {
  const char *name;
  yr_test_suite_collection_t (*collection_creator)(struct setup_teardown_test_data **refcon);
} name_map[] = {
  { "do_test_basics_setups_teardowns", create_setups_teardowns_suite_collection },
  { "do_test_aborts", create_setups_teardowns_aborts_suite_collection },
  { "do_test_suspension", create_test_suspension_suite_collection },
  { "intermediate", create_test_suspension_final_suite_collection },
};

static yr_test_suite_collection_t (*find_creator(const char *name))(struct setup_teardown_test_data **refcon)
{
  for ( size_t i = 0; i < sizeof(name_map) / sizeof(name_map[0]); i++ ) {
    if ( strcmp(name_map[i].name, name) == 0 ) {
      return name_map[i].collection_creator;
    }
  }
  return NULL;
}

extern char **environ;

static void intermediate(yr_test_case_t testcase)
{
  yr_suspend_inferiority();
  setenv(env_key, __FUNCTION__, 1);
  yr_test_suite_collection_t collection = find_creator(__FUNCTION__)(NULL);
  yr_result_store_t store = yr_result_store_create(__FUNCTION__);
  yr_run_suite_collection_under_store_multiprocess(proc_info.argv[0], proc_info.argv, environ,
                                                   collection, store, YR_BASIC_STDERR_RUNTIME_CALLBACKS);
  yr_result_store_close(store);
  YR_ASSERT_EQUAL(yr_result_store_count_subresults(store), 1);
  yr_result_store_t suite_store = yr_result_store_get_subresult(store, 0);
  YR_ASSERT_EQUAL(yr_result_store_count_subresults(suite_store), 2);
  YR_ASSERT_EQUAL(yr_result_store_get_result(yr_result_store_get_subresult(suite_store, 0)), YR_RESULT_PASSED);
  YR_ASSERT_EQUAL(yr_result_store_get_result(yr_result_store_get_subresult(suite_store, 1)), YR_RESULT_SKIPPED);
  YR_ASSERT_EQUAL(yr_result_store_get_result(suite_store), YR_RESULT_PASSED);
  YR_ASSERT_EQUAL(yr_result_store_get_result(store), YR_RESULT_PASSED);
  yr_result_store_destroy(store);
  free(collection);
  yr_reset_inferiority();
  YR_ASSERT(yr_process_is_inferior());
}

void do_test_basics_setups_teardowns(yr_test_case_t tc)
{
  setenv(env_key, __FUNCTION__, 1);
  struct setup_teardown_test_data *data;
  yr_test_suite_collection_t collection = find_creator(__FUNCTION__)(&data);
  struct proc_info *info = tc->suite->refcon;
  yr_result_store_t store = yr_result_store_create(__FUNCTION__);
  yr_run_suite_collection_under_store_multiprocess(info->argv[0], info->argv, environ,
                                                   collection, store,
                                                   YR_BASIC_STDERR_RUNTIME_CALLBACKS);
  YR_ASSERT(yr_result_store_count_subresults(store) == 2);
  yr_result_store_t first_suite = yr_result_store_get_subresult(store, 0);
  YR_ASSERT(yr_result_store_count_subresults(first_suite) == 2);
  YR_ASSERT(yr_result_store_get_result(yr_result_store_get_subresult(first_suite, 0)) == YR_RESULT_PASSED);
  YR_ASSERT(yr_result_store_get_result(yr_result_store_get_subresult(first_suite, 1)) == YR_RESULT_PASSED);
  YR_ASSERT(yr_result_store_get_result(first_suite) == YR_RESULT_PASSED);
  yr_result_store_t second_suite = yr_result_store_get_subresult(store, 1);
  YR_ASSERT(yr_result_store_count_subresults(second_suite) == 1);
  YR_ASSERT(yr_result_store_get_result(yr_result_store_get_subresult(second_suite, 0)) == YR_RESULT_FAILED);
  YR_ASSERT(yr_result_store_get_result(second_suite) == YR_RESULT_FAILED);
  YR_ASSERT(yr_result_store_get_result(store) == YR_RESULT_FAILED);
  YR_ASSERT(!yr_result_store_is_closed(store));
  yr_result_store_destroy(store);
  free(collection);
  free(data);
}

void do_test_aborts(yr_test_case_t tc)
{
  setenv(env_key, __FUNCTION__, 1);
  struct setup_teardown_test_data *data;
  yr_test_suite_collection_t collection = find_creator(__FUNCTION__)(&data);
  struct proc_info *info = tc->suite->refcon;
  yr_result_store_t store = yr_result_store_create(__FUNCTION__);
  yr_run_suite_collection_under_store_multiprocess(info->argv[0], info->argv, environ,
                                                   collection, store,
                                                   YR_BASIC_STDERR_RUNTIME_CALLBACKS);
  YR_ASSERT(yr_result_store_count_subresults(store) == 2);
  yr_result_store_t first_suite = yr_result_store_get_subresult(store, 0);
  YR_ASSERT(yr_result_store_count_subresults(first_suite) == 4);
  YR_ASSERT(yr_result_store_get_result(yr_result_store_get_subresult(first_suite, 0)) == YR_RESULT_PASSED);
  YR_ASSERT(yr_result_store_get_result(yr_result_store_get_subresult(first_suite, 1)) == YR_RESULT_FAILED);
  YR_ASSERT(yr_result_store_get_result(yr_result_store_get_subresult(first_suite, 2)) == YR_RESULT_PASSED);
  YR_ASSERT(yr_result_store_get_result(yr_result_store_get_subresult(first_suite, 3)) == YR_RESULT_PASSED);
  YR_ASSERT(yr_result_store_get_result(first_suite) == YR_RESULT_FAILED);
  yr_result_store_t second_suite = yr_result_store_get_subresult(store, 1);
  YR_ASSERT(yr_result_store_count_subresults(second_suite) == 1);
  YR_ASSERT(yr_result_store_get_result(yr_result_store_get_subresult(second_suite, 0)) == YR_RESULT_PASSED);
  YR_ASSERT(yr_result_store_get_result(second_suite) == YR_RESULT_PASSED);
  YR_ASSERT(yr_result_store_get_result(store) == YR_RESULT_FAILED);
  YR_ASSERT(!yr_result_store_is_closed(store));
  yr_result_store_destroy(store);
  free(collection);
  free(data);
}

void do_test_suspension(yr_test_case_t tc)
{
  setenv(env_key, __FUNCTION__, 1);
  yr_test_suite_collection_t collection = find_creator(__FUNCTION__)(NULL);
  struct proc_info *info = tc->suite->refcon;
  yr_result_store_t store = yr_result_store_create(__FUNCTION__);
  yr_run_suite_collection_under_store_multiprocess(info->argv[0], info->argv, environ, collection, store,
                                                   YR_BASIC_STDERR_RUNTIME_CALLBACKS);
  yr_result_store_close(store);
  YR_ASSERT_EQUAL(yr_result_store_get_result(store), YR_RESULT_PASSED);

  yr_result_store_destroy(store);
  free(collection);
}

int main(int argc, char **argv)
{
  proc_info.argc = argc;
  proc_info.argv = argv;

  if ( yr_process_is_inferior() ) {
    struct setup_teardown_test_data *data;
    yr_test_suite_collection_t collection = find_creator(getenv(env_key))(&data);
    yr_inferior_checkin(collection, YR_BASIC_STDERR_RUNTIME_CALLBACKS);
    exit(EXIT_FAILURE); // shouldn't get here
  }
  yr_test_suite_t suite = yr_create_suite_from_functions("basic tests", &proc_info, YR_NO_CALLBACKS,
                                                         do_test_basics_setups_teardowns,
                                                         do_test_aborts,
                                                         do_test_suspension);
  if ( yr_basic_run_suite(suite) ) {
    fprintf(stderr, "some tests failed!\n");
    return EXIT_FAILURE;
  }
  free(suite);
  return 0;
}

#include <yachtrock/yachtrock.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/resource.h>

#include "multiprocess_tests.h"

static char *get_tramp_path(void)
{
  char *result = getenv(TEST_TRAMPOLINE_PATH_VAR);
  if ( result == NULL ) {
    fprintf(stderr, "couldn't get path to trampoline from env var %s\n", TEST_TRAMPOLINE_PATH_VAR);
    abort();
  }
  return result;
}

static void prep_argv(char *argv[static 2])
{
  argv[0] = get_tramp_path();
  argv[1] = NULL;
}

static const char *env_key = INFERIOR_COLLECTION_NAME_VAR;

struct setup_teardown_test_data
{
  unsigned case_setups_run;
  unsigned case_teardowns_run;
  unsigned suite_setups_run;
  unsigned suite_teardowns_run;
  unsigned num_cases_run;
};
static void setup_test_suite_setup(yr_test_suite_t suite)
{
  ((struct setup_teardown_test_data *)suite->refcon)->suite_setups_run++;
}
static void alloc_setup_teardown_test_data_then_setup_test_suite_setup(yr_test_suite_t suite)
{
  struct setup_teardown_test_data *data = malloc(sizeof(struct setup_teardown_test_data));
  *data = (struct setup_teardown_test_data){0};
  suite->refcon = data;
  setup_test_suite_setup(suite);
}
static void fetch_refcon_from_indirect_ptr_then_setup_test_suite_setup(yr_test_suite_t suite)
{
  suite->refcon = *(void **)suite->refcon;
  setup_test_suite_setup(suite);
}
static void fetch_refcon_from_indirect_ptr_alloc_if_null_then_setup_test_suite_setup(yr_test_suite_t suite)
{
  suite->refcon = *(void **)suite->refcon;
  if ( suite->refcon == NULL ) {
    struct setup_teardown_test_data *data = malloc(sizeof(struct setup_teardown_test_data));
    *data = (struct setup_teardown_test_data){0};
    suite->refcon = data;
  }
  setup_test_suite_setup(suite);
}

static void setup_test_suite_teardown(yr_test_suite_t suite)
{
  ((struct setup_teardown_test_data *)suite->refcon)->suite_teardowns_run++;
}
static void setup_test_suite_teardown_then_dealloc_refcon(yr_test_suite_t suite)
{
  setup_test_suite_teardown(suite);
  free(suite->refcon);
  suite->refcon = NULL;
}
static void setup_test_case_setup(yr_test_case_t testcase)
{
  ((struct setup_teardown_test_data *)testcase->suite->refcon)->case_setups_run++;
}
static void setup_test_case_teardown(yr_test_case_t testcase)
{
  ((struct setup_teardown_test_data *)testcase->suite->refcon)->case_teardowns_run++;
}
static void test_suite_and_case_setups_run_pt1(yr_test_case_t testcase)
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
static void test_suite_and_case_setups_run_pt2(yr_test_case_t testcase)
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
static void test_suite_and_case_setups_run_pt3(yr_test_case_t testcase)
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

static yr_test_suite_collection_t create_setups_teardowns_suite_collection()
{
  yr_test_suite_t suite1 = calloc(sizeof(yr_test_suite_s) + sizeof(yr_test_case_s) * 2, 1);
  suite1->name = "Basic setup/teardown test";
  suite1->num_cases = 2;
  suite1->lifecycle.setup_case = setup_test_case_setup;
  suite1->lifecycle.teardown_case = setup_test_case_teardown;
  suite1->lifecycle.setup_suite = alloc_setup_teardown_test_data_then_setup_test_suite_setup;
  suite1->lifecycle.teardown_suite = setup_test_suite_teardown;
  suite1->cases[0].name = "test_suite_and_case_setups_run_pt1";
  suite1->cases[0].testcase = test_suite_and_case_setups_run_pt1;
  suite1->cases[0].suite = suite1;
  suite1->cases[1].name = "test_suite_and_case_setups_run_pt2";
  suite1->cases[1].testcase = test_suite_and_case_setups_run_pt2;
  suite1->cases[1].suite = suite1;

  yr_test_suite_t suite2 = calloc(sizeof(yr_test_suite_s) + sizeof(yr_test_case_s), 1);
  suite2->name = "basic setup/teardown test";
  suite2->refcon = NULL;
  suite2->num_cases = 1;
  suite2->lifecycle.setup_case = setup_test_case_setup;
  suite2->lifecycle.teardown_case = setup_test_case_teardown;
  suite2->lifecycle.setup_suite = fetch_refcon_from_indirect_ptr_then_setup_test_suite_setup;
  suite2->lifecycle.teardown_suite = setup_test_suite_teardown_then_dealloc_refcon;
  suite2->cases[0].name = "test_suite_and_case_setups_run_pt3";
  suite2->cases[0].testcase = test_suite_and_case_setups_run_pt3;
  suite2->cases[0].suite = suite2;

  yr_test_suite_t suites[] = {suite1, suite2};

  yr_test_suite_collection_t collection = yr_test_suite_collection_create_from_suites(2, suites);
  collection->suites[1]->refcon = &(collection->suites[0]->refcon);
  free(suite1);
  free(suite2);
  return collection;
}

static YR_TESTCASE(test_aborting_pt1)
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

static YR_TESTCASE(test_aborting_pt2)
{
  abort();
}

static YR_TESTCASE(test_aborting_pt3)
{
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->case_setups_run == 1);
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->case_teardowns_run == 0);
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->suite_setups_run == 1);
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->suite_teardowns_run == 0);
  ((struct setup_teardown_test_data *)testcase->suite->refcon)->num_cases_run++;
}

static YR_TESTCASE(test_aborting_pt4)
{
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->case_setups_run == 2);
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->case_teardowns_run == 1);
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->suite_setups_run == 1);
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->suite_teardowns_run == 0);
  ((struct setup_teardown_test_data *)testcase->suite->refcon)->num_cases_run++;
}

static YR_TESTCASE(test_aborting_pt5)
{
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->case_setups_run == 3);
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->case_teardowns_run == 2);
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->suite_setups_run == 2);
  YR_ASSERT(((struct setup_teardown_test_data *)testcase->suite->refcon)->suite_teardowns_run == 1);
  ((struct setup_teardown_test_data *)testcase->suite->refcon)->num_cases_run++;
}

static yr_test_suite_collection_t create_setups_teardowns_aborts_suite_collection(void)
{
  yr_test_suite_t suite1 = calloc(sizeof(yr_test_suite_s) + sizeof(yr_test_case_s) * 4, 1);
  suite1->name = "aborting test suite1";
  suite1->refcon = NULL;
  suite1->num_cases = 4;
  suite1->lifecycle.setup_case = setup_test_case_setup;
  suite1->lifecycle.teardown_case = setup_test_case_teardown;
  suite1->lifecycle.setup_suite = alloc_setup_teardown_test_data_then_setup_test_suite_setup;
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
  suite2->refcon = NULL;
  suite2->num_cases = 1;
  suite2->lifecycle.setup_case = setup_test_case_setup;
  suite2->lifecycle.teardown_case = setup_test_case_teardown;
  suite2->lifecycle.setup_suite = fetch_refcon_from_indirect_ptr_alloc_if_null_then_setup_test_suite_setup;
  suite2->lifecycle.teardown_suite = setup_test_suite_teardown_then_dealloc_refcon;
  suite2->cases[0].name = "test_aborting_pt5";
  suite2->cases[0].testcase = test_aborting_pt5;
  suite2->cases[0].suite = suite2;

  yr_test_suite_t suites[] = {suite1, suite2};

  yr_test_suite_collection_t collection = yr_test_suite_collection_create_from_suites(2, suites);
  collection->suites[1]->refcon = &(collection->suites[0]->refcon);
  free(suite1);
  free(suite2);
  return collection;
}

static YR_TESTCASE(dummy1) { }
static YR_TESTCASE(dummy2)
{
  YR_RECORD_SKIP("because I said so");
}

static yr_test_suite_collection_t create_test_suspension_final_suite_collection(void)
{
  yr_test_suite_t suite1 = yr_create_suite_from_functions("suspension final suite", NULL, YR_NO_CALLBACKS,
                                                          dummy1, dummy2);
  yr_test_suite_collection_t collection = yr_test_suite_collection_create_from_suites(1, &suite1);
  free(suite1);
  return collection;
}

static void intermediate(yr_test_case_t testcase);

static yr_test_suite_collection_t create_test_suspension_suite_collection(void)
{
  yr_test_suite_t suite1 = yr_create_suite_from_functions("intermediate suite", NULL, YR_NO_CALLBACKS,
                                                          intermediate);
  yr_test_suite_collection_t collection = yr_test_suite_collection_create_from_suites(1, &suite1);
  free(suite1);
  return collection;
}

static void fork_subprocess_and_abort(yr_test_case_t testcase)
{
  if ( fork() == 0 ) {
    static char * const argv[] = {
      "/bin/sh",
      "-c",
      "sleep 3600",
      NULL
    };
    execv(argv[0], argv);
  } else {
    fprintf(stderr, "aieeee!\n");
    abort();
  }
}

static yr_test_suite_collection_t create_test_abort_with_subprocess_collection(void)
{
  yr_test_suite_t suite = yr_create_suite_from_functions("forking suite",
                                                         NULL, YR_NO_CALLBACKS,
                                                         fork_subprocess_and_abort);
  yr_test_suite_collection_t collection = yr_test_suite_collection_create_from_suites(1, &suite);
  free(suite);
  return collection;
}

static const uint32_t lorge_num_suites = 1000;
static const uint32_t lorge_num_cases = 1000;
static void case_f(yr_test_case_t testcase) {}
static yr_test_suite_collection_t create_lorge_collection(void)
{
  yr_test_suite_t *suites = calloc(sizeof(yr_test_suite_t), lorge_num_suites);
  for ( uint32_t suite_i = 0; suite_i < lorge_num_suites; suite_i++ ) {
    char _;
    yr_test_suite_t suite = yr_create_blank_suite(lorge_num_cases);
    for ( uint32_t case_i = 0; case_i < lorge_num_cases; case_i++ ) {
      char *case_name_fmt = "suite %ul case %ul";
      int necessary = snprintf(&_, 0, case_name_fmt, (unsigned long)suite_i, (unsigned long)case_i) + 1;
      suite->cases[case_i].name = malloc(necessary);
      suite->cases[case_i].testcase = case_f;
      /* const cast is valid, allocated from free store */
      snprintf((char *)suite->cases[case_i].name, necessary, case_name_fmt, (unsigned long)suite_i, (unsigned long)case_i);
    }

    char *suite_name_fmt = "suite %ul";
    int necessary = snprintf(&_, 0, suite_name_fmt, (unsigned long)suite_i);
    suite->name = malloc(necessary);
    /* const cast is valid, allocated from free store */
    snprintf((char *)suite->name, necessary, suite_name_fmt, (unsigned long)suite_i);
    suites[suite_i] = suite;
  }

  yr_test_suite_collection_t collection = yr_test_suite_collection_create_from_suites(lorge_num_suites,
                                                                                      suites);
  for ( uint32_t suite_i = 0; suite_i < lorge_num_suites; suite_i++ ) {
    yr_test_suite_t suite = suites[suite_i];
    assert(suite);
    for ( uint32_t case_i = 0; case_i < lorge_num_cases; case_i++ ) {
      /* const cast is valid, allocated from free store */
      free((char *)suite->cases[case_i].name);
    }
    /* const cast is valid, allocated from free store */
    free((char *)suite->name);
    free(suite);
  }
  free(suites);

  return collection;
}

static const struct {
  const char *name;
  yr_test_suite_collection_t (*collection_creator)(void);
} name_map[] = {
  { "do_test_basics_setups_teardowns", create_setups_teardowns_suite_collection },
  { "do_test_aborts", create_setups_teardowns_aborts_suite_collection },
  { "do_test_suspension", create_test_suspension_suite_collection },
  { "intermediate", create_test_suspension_final_suite_collection },
  { "do_test_abort_with_forked_subprocess", create_test_abort_with_subprocess_collection },
  { "do_test_big_collections", create_lorge_collection },
};

static yr_test_suite_collection_t (*find_creator(const char *name))(void)
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
  yr_test_suite_collection_t collection = find_creator(__FUNCTION__)();
  yr_result_store_t store = yr_result_store_create(__FUNCTION__);
  char *argv[2];
  prep_argv(argv);
  yr_run_suite_collection_under_store_multiprocess(argv[0], argv, environ,
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

static void do_test_basics_setups_teardowns(yr_test_case_t tc)
{
  setenv(env_key, __FUNCTION__, 1);
  yr_test_suite_collection_t collection = find_creator(__FUNCTION__)();
  yr_result_store_t store = yr_result_store_create(__FUNCTION__);
  char *argv[2];
  prep_argv(argv);
  yr_run_suite_collection_under_store_multiprocess(argv[0], argv, environ,
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
}

static void do_test_aborts(yr_test_case_t tc)
{
  setenv(env_key, __FUNCTION__, 1);
  yr_test_suite_collection_t collection = find_creator(__FUNCTION__)();
  yr_result_store_t store = yr_result_store_create(__FUNCTION__);
  char *argv[2];
  prep_argv(argv);
  yr_run_suite_collection_under_store_multiprocess(argv[0], argv, environ,
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
}

static bool all_passes(yr_result_store_t store)
{
  if ( yr_result_store_get_result(store) != YR_RESULT_PASSED ) {
    return false;
  }
  for ( size_t i = 0; i < yr_result_store_count_subresults(store); i++ ) {
    if ( !all_passes(yr_result_store_get_subresult(store, i)) ) {
      return false;
    }
  }
  return true;
}

static void do_test_suspension(yr_test_case_t tc)
{
  setenv(env_key, __FUNCTION__, 1);
  yr_test_suite_collection_t collection = find_creator(__FUNCTION__)();
  yr_result_store_t store = yr_result_store_create(__FUNCTION__);
  char *argv[2];
  prep_argv(argv);
  yr_run_suite_collection_under_store_multiprocess(argv[0], argv, environ, collection, store,
                                                   YR_BASIC_STDERR_RUNTIME_CALLBACKS);
  yr_result_store_close(store);
  YR_ASSERT_EQUAL(yr_result_store_get_result(store), YR_RESULT_PASSED);
  YR_ASSERT(all_passes(store));

  yr_result_store_destroy(store);
  free(collection);
}

static void do_test_abort_with_forked_subprocess(yr_test_case_t tc)
{
  setenv(env_key, __FUNCTION__, 1);
  yr_test_suite_collection_t collection = find_creator(__FUNCTION__)();
  yr_result_store_t store = yr_result_store_create(__FUNCTION__);
  char *argv[2];
  prep_argv(argv);

  yr_run_suite_collection_under_store_multiprocess(argv[0], argv, environ, collection, store,
                                                   YR_BASIC_STDERR_RUNTIME_CALLBACKS);
  yr_result_store_close(store);
  YR_ASSERT_EQUAL(yr_result_store_get_result(store), YR_RESULT_FAILED);

  yr_result_store_destroy(store);
  free(collection);
}

static void suite_setup_suspend_inferiority_no_cores(yr_test_suite_t suite)
{
  struct rlimit *core = malloc(sizeof(struct rlimit));
  assert(core);
  if ( getrlimit(RLIMIT_CORE, core) == 0 ) {
    suite->refcon = core;
    struct rlimit newcore = *core;
    newcore.rlim_cur = 0;
    if ( setrlimit(RLIMIT_CORE, &newcore) < 0 ) {
      perror("setrlimit (set no cores)");
    }
  } else {
    perror("getrlimit");
  }
  yr_suspend_inferiority();
}
static void suite_teardown_reset_inferiority_reset_cores(yr_test_suite_t suite)
{
  if ( suite->refcon != NULL ) {
    struct rlimit *oldlimit = suite->refcon;
    if ( setrlimit(RLIMIT_CORE, oldlimit) < 0 ) {
      perror("setrlimit (restore cores)");
    }
    free(suite->refcon);
    suite->refcon = NULL;
  }
  yr_reset_inferiority();
}

yr_test_suite_collection_t yr_multiprocess_test_inferior_create_collection(const char *collname)
{
  return find_creator(collname)();
}

static void store_closed_big_collections(yr_result_store_t store, void *refcon)
{
  if ( yr_result_store_count_subresults(store) > 0 && yr_result_store_get_parent(store) != NULL ) {
    fprintf(stderr, ".");
  } else if ( yr_result_store_get_parent(store) == NULL ) {
    fprintf(stderr, "\n");
  }
}

static void do_test_big_collections(yr_test_case_t testcase)
{
  setenv(env_key, __FUNCTION__, 1);
  yr_test_suite_collection_t collection = find_creator(__FUNCTION__)();
  struct yr_result_hooks result_hooks = {
    .store_closed = store_closed_big_collections,
  };
  yr_result_store_t store = yr_result_store_create_with_hooks(__FUNCTION__, result_hooks);
  char *argv[2];
  prep_argv(argv);
  yr_run_suite_collection_under_store_multiprocess(argv[0], argv, environ, collection, store,
                                                   YR_BASIC_STDERR_RUNTIME_CALLBACKS);
  yr_result_store_close(store);
  YR_ASSERT_EQUAL(yr_result_store_get_result(store), YR_RESULT_PASSED);
  YR_ASSERT(all_passes(store));
  yr_result_store_destroy(store);
  free(collection);
}

yr_test_suite_t yr_create_multiprocess_suite(void)
{
  struct yr_suite_lifecycle_callbacks callbacks = {0};
  callbacks.setup_suite = suite_setup_suspend_inferiority_no_cores;
  callbacks.teardown_suite = suite_teardown_reset_inferiority_reset_cores;

  struct multiprocess_testcase {
    void (*testcase)(yr_test_case_t testcase);
    char *name;
  };

#define ENTRY(f) { .testcase = f, .name = #f }

  struct multiprocess_testcase always_cases[] = {
    ENTRY(do_test_basics_setups_teardowns),
    ENTRY(do_test_aborts),
    ENTRY(do_test_suspension),
    ENTRY(do_test_abort_with_forked_subprocess),
  };

  size_t num_always_cases = sizeof(always_cases) / sizeof(always_cases[0]);
  size_t num_cases = num_always_cases;

  if ( getenv("YR_LORGE") ) {
    num_cases++;
  }

  yr_test_suite_t suite = yr_create_blank_suite(num_cases);
  for ( size_t i = 0; i < num_always_cases; i++ ) {
    suite->cases[i].name = always_cases[i].name;
    suite->cases[i].testcase = always_cases[i].testcase;
  }

  if ( getenv("YR_LORGE") ) {
    suite->cases[num_always_cases].name = "do_test_big_collections";
    suite->cases[num_always_cases].testcase = do_test_big_collections;
  }

  suite->name = "multiprocess basic tests";
  suite->lifecycle = callbacks;

  return suite;
}

#include <yachtrock/yachtrock.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if YACHTROCK_DLOPEN
#include <dlfcn.h>
#endif

struct testcase_tests_suite_context {
  const char *dummy_module_path;
};

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
                                                         NULL, callbacks,
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
                                                         NULL, callbacks,
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
                                                          NULL, callbacks1,
                                                          dummy1, dummy2, dummy3);
  yr_test_suite_t suite2 = yr_create_suite_from_functions("suite2",
                                                          NULL, callbacks2,
                                                          dummy12, dummy22, dummy32);
  yr_test_suite_t suite3 = yr_create_suite_from_functions("suite3",
                                                          NULL, callbacks3,
                                                          dummy12, dummy2);
  yr_test_suite_t suite4 = yr_create_suite_from_functions("suite4",
                                                          NULL, YR_NO_CALLBACKS,
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

static YR_TESTCASE(test_collection_from_collections)
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
                                                          NULL, callbacks1,
                                                          dummy1, dummy2, dummy3);
  yr_test_suite_t suite2 = yr_create_suite_from_functions("suite2",
                                                          NULL, callbacks2,
                                                          dummy12, dummy22, dummy32);
  yr_test_suite_t suite3 = yr_create_suite_from_functions("suite3",
                                                          NULL, callbacks3,
                                                          dummy12, dummy2);
  yr_test_suite_t suite4 = yr_create_suite_from_functions("suite4",
                                                          NULL, YR_NO_CALLBACKS,
                                                          dummy1, dummy22, dummy32, dummy12);

  yr_test_suite_t suites1[] = { suite1, suite2 };
  yr_test_suite_t suites2[] = { suite3 };
  yr_test_suite_t suites3[] = { suite4 };
  yr_test_suite_collection_t collection1 = yr_test_suite_collection_create_from_suites(2, suites1);
  yr_test_suite_collection_t collection2 = yr_test_suite_collection_create_from_suites(1, suites2);
  yr_test_suite_collection_t collection3 = yr_test_suite_collection_create_from_suites(1, suites3);

  yr_test_suite_collection_t final = yr_test_suite_collection_create_from_collections(3,
                                                                                      collection1,
                                                                                      collection2,
                                                                                      collection3);
  free(collection1);
  free(collection2);
  free(collection3);

  yr_test_suite_t all_suites[] = {suite1, suite2, suite3, suite4};
  YR_ASSERT_EQUAL(final->num_suites, 4);
  for ( size_t i = 0; i < 4; i++ ) {
    assert_suites_equal_but_independent(all_suites[i], final->suites[i]);
    free(all_suites[i]);
  }
  free(final);
}

#if YACHTROCK_DLOPEN

static YR_TESTCASE(test_collection_discovery)
{
  struct testcase_tests_suite_context *context = testcase->suite->refcon;
  char *err = NULL;
  yr_test_suite_collection_t collection =
    yr_test_suite_collection_load_from_dylib_path(context->dummy_module_path,
                                                  &err);
  YR_ASSERT(collection != NULL, "didn't get collection, error is %s", err);
  YR_ASSERT_EQUAL(collection->num_suites, 2);
  YR_ASSERT_EQUAL(collection->suites[0]->num_cases, 2);
  YR_ASSERT_EQUAL(collection->suites[1]->num_cases, 2);

  yr_result_store_t store = yr_result_store_create(__FUNCTION__);
  yr_run_suite_collection_under_store(collection, store, (struct yr_runtime_callbacks){0});
  yr_result_store_close(store);
  YR_ASSERT_EQUAL(yr_result_store_get_result(store), YR_RESULT_PASSED);
  yr_result_store_destroy(store);
  free(collection);
}

static YR_TESTCASE(test_collection_discovery_handle)
{
  struct testcase_tests_suite_context *context = testcase->suite->refcon;
  char *err = NULL;
  void *handle = dlopen(context->dummy_module_path, RTLD_LAZY);
  YR_ASSERT(handle != NULL, "Couldn't load handle: %s", dlerror());
  yr_test_suite_collection_t collection =
    yr_test_suite_collection_load_from_handle(handle,
                                              &err);
  YR_ASSERT(collection != NULL, "Didn't get collection, error is %s", err);
  YR_ASSERT_EQUAL(collection->num_suites, 2);
  YR_ASSERT_EQUAL(collection->suites[0]->num_cases, 2);
  YR_ASSERT_EQUAL(collection->suites[1]->num_cases, 2);

  yr_result_store_t store = yr_result_store_create(__FUNCTION__);
  yr_run_suite_collection_under_store(collection, store, (struct yr_runtime_callbacks){0});
  yr_result_store_close(store);
  YR_ASSERT_EQUAL(yr_result_store_get_result(store), YR_RESULT_PASSED);
  yr_result_store_destroy(store);
  free(collection);
  dlclose(handle);
}

static YR_TESTCASE(test_collection_discovery_fail_no_dylib)
{
  char *err = NULL;
  yr_test_suite_collection_t collection =
    yr_test_suite_collection_load_from_dylib_path("nope.dylib",
                                                  &err);
  YR_ASSERT_EQUAL(collection, NULL);
  YR_ASSERT(err != NULL);
  free(err);
}

#endif // YACHTROCK_DLOPEN

static YR_TESTCASE(placeholder_test) {}

int main(int argc, char **argv)
{
  if ( argc < 2 ) {
    fprintf(stderr, "not passed dummy module name?\n");
    abort();
  }
  struct testcase_tests_suite_context context;
  context.dummy_module_path = argv[1];
  yr_test_suite_t suite = yr_create_suite_from_functions("testcase tests", &context,
                                                         YR_NO_CALLBACKS,
                                                         test_create_from_functions,
                                                         test_collection_from_suites_basic,
                                                         test_collection_from_suites_more,
                                                         test_collection_from_collections,
#if YACHTROCK_DLOPEN
                                                         test_collection_discovery,
                                                         test_collection_discovery_handle,
                                                         test_collection_discovery_fail_no_dylib,
#endif
                                                         placeholder_test
                                                         );
  if ( yr_basic_run_suite(suite) ) {
    fprintf(stderr, "some tests failed!\n");
    return EXIT_FAILURE;
  }
  free(suite);
  return 0;

}

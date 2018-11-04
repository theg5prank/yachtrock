#include <yachtrock/yachtrock.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct run_under_result_store_store_context
{
  bool dummy1_seen_open;
  bool dummy1_seen_pass;
  bool dummy1_seen_close;
  bool dummy2_seen_open;
  bool dummy2_seen_pass;
  bool dummy2_seen_close;
  bool dummy3_seen_open;
  bool dummy3_seen_fail;
  bool dummy3_seen_close;
};

struct run_under_result_store_suite_context
{
  uint64_t rutabega[2];
  bool dummy1_seen_opened_hook;
  bool dummy1_seen_case;
  bool dummy1_seen_closed_hook;
  bool dummy2_seen_opened_hook;
  bool dummy2_seen_case;
  bool dummy2_seen_closed_hook;
  bool dummy3_seen_opened_hook;
  bool dummy3_seen_case;
  bool dummy3_seen_closed_hook;
};

static YR_TESTCASE(dummy1)
{
  ((struct run_under_result_store_suite_context *)testcase->suite->refcon)->dummy1_seen_case = true;
}
static YR_TESTCASE(dummy2)
{
  ((struct run_under_result_store_suite_context *)testcase->suite->refcon)->dummy2_seen_case = true;
}
static YR_TESTCASE(dummy3)
{
  ((struct run_under_result_store_suite_context *)testcase->suite->refcon)->dummy3_seen_case = true;
  YR_FAIL("Failed for totally expected reasons!");
}
static void case_opened_callback(yr_test_case_t testcase)
{
  if ( testcase->testcase == dummy1 && strcmp(testcase->name, "dummy1") == 0 ) {
    ((struct run_under_result_store_suite_context *)testcase->suite->refcon)->dummy1_seen_opened_hook = true;
  } else if ( testcase->testcase == dummy2 && strcmp(testcase->name, "dummy2") == 0 ) {
    ((struct run_under_result_store_suite_context *)testcase->suite->refcon)->dummy2_seen_opened_hook = true;
  } else if ( testcase->testcase == dummy3 && strcmp(testcase->name, "dummy3") == 0 ) {
    ((struct run_under_result_store_suite_context *)testcase->suite->refcon)->dummy3_seen_opened_hook = true;
  }
}
static void case_closed_callback(yr_test_case_t testcase)
{
  if ( testcase->testcase == dummy1 && strcmp(testcase->name, "dummy1") == 0 ) {
    ((struct run_under_result_store_suite_context *)testcase->suite->refcon)->dummy1_seen_closed_hook = true;
  } else if ( testcase->testcase == dummy2 && strcmp(testcase->name, "dummy2") == 0 ) {
    ((struct run_under_result_store_suite_context *)testcase->suite->refcon)->dummy2_seen_closed_hook = true;
  } else if ( testcase->testcase == dummy3 && strcmp(testcase->name, "dummy3") == 0 ) {
    ((struct run_under_result_store_suite_context *)testcase->suite->refcon)->dummy3_seen_closed_hook = true;
  }
}
static void store_opened_callback(yr_result_store_t new_store, void *refcon)
{
  size_t num_hops_to_root = 0;
  yr_result_store_t parent = new_store;
  while ( (parent = yr_result_store_get_parent(parent)) ) {
    num_hops_to_root++;
  }
  if ( num_hops_to_root != 2 ) {
    return;
  }
  struct run_under_result_store_store_context *context = refcon;
  if ( strcmp(yr_result_store_get_name(new_store), "dummy1") == 0 ) {
    context->dummy1_seen_open = true;
  } else if ( strcmp(yr_result_store_get_name(new_store), "dummy2") == 0 ) {
    context->dummy2_seen_open = true;
  } if ( strcmp(yr_result_store_get_name(new_store), "dummy3") == 0 ) {
    context->dummy3_seen_open = true;
  }
}
static void store_closed_callback(yr_result_store_t closed_store, void *refcon)
{
  size_t num_hops_to_root = 0;
  yr_result_store_t parent = closed_store;
  while ( (parent = yr_result_store_get_parent(parent)) ) {
    num_hops_to_root++;
  }
  if ( num_hops_to_root != 2 ) {
    return;
  }
  struct run_under_result_store_store_context *context = refcon;
  if ( strcmp(yr_result_store_get_name(closed_store), "dummy1") == 0 ) {
    context->dummy1_seen_close = true;
  } else if ( strcmp(yr_result_store_get_name(closed_store), "dummy2") == 0 ) {
    context->dummy2_seen_close = true;
  } if ( strcmp(yr_result_store_get_name(closed_store), "dummy3") == 0 ) {
    context->dummy3_seen_close = true;
  }
}
static void store_changed_callback(yr_result_store_t store, void *refcon)
{
  size_t num_hops_to_root = 0;
  yr_result_store_t parent = store;
  while ( (parent = yr_result_store_get_parent(parent)) ) {
    num_hops_to_root++;
  }
  if ( num_hops_to_root != 2 ) {
    return;
  }
  struct run_under_result_store_store_context *context = refcon;
  if ( strcmp(yr_result_store_get_name(store), "dummy1") == 0 ) {
    context->dummy1_seen_pass = yr_result_store_get_result(store) == YR_RESULT_PASSED;
  } else if ( strcmp(yr_result_store_get_name(store), "dummy2") == 0 ) {
    context->dummy2_seen_pass = yr_result_store_get_result(store) == YR_RESULT_PASSED;
  } if ( strcmp(yr_result_store_get_name(store), "dummy3") == 0 ) {
    context->dummy3_seen_fail = yr_result_store_get_result(store) == YR_RESULT_FAILED;
  }
}

struct store_and_suite_context
{
  yr_test_suite_t suite;
  yr_result_store_t store;
  struct run_under_result_store_store_context *store_context;
  struct run_under_result_store_suite_context *suite_context;
};
static void setup_store_and_suite(yr_test_case_t testcase)
{
  struct store_and_suite_context *context = malloc(sizeof(struct store_and_suite_context));
  struct yr_suite_lifecycle_callbacks lifecycle_callbacks = {0};
  lifecycle_callbacks.setup_case = case_opened_callback;
  lifecycle_callbacks.teardown_case = case_closed_callback;
  context->suite_context = malloc(sizeof(struct run_under_result_store_suite_context));
  *(context->suite_context) = (struct run_under_result_store_suite_context){{0}};
  context->suite = yr_create_suite_from_functions("dummy suite",
                                                  context->suite_context,
                                                  lifecycle_callbacks,
                                                  dummy1,
                                                  dummy2,
                                                  dummy3);
  struct yr_result_hooks store_hooks;
  store_hooks.store_opened = store_opened_callback;
  store_hooks.store_closed = store_closed_callback;
  store_hooks.store_result_changed = store_changed_callback;
  context->store_context = malloc(sizeof(struct run_under_result_store_store_context));
  *(context->store_context) = (struct run_under_result_store_store_context){0};
  context->store = yr_result_store_create_with_hooks("root store",
                                                     store_hooks, context->store_context);
  ((struct yr_test_suite *)testcase->suite)->refcon = context;
}
static void teardown_store_and_suite(yr_test_case_t testcase)
{
  struct store_and_suite_context *context = testcase->suite->refcon;
  free(context->suite);
  yr_result_store_destroy(context->store);
  free(context->store_context);
  free(context->suite_context);
  free(context);
  ((struct yr_test_suite *)testcase->suite)->refcon = NULL;
}
static YR_TESTCASE(test_run_under_store_callbacks)
{
  struct store_and_suite_context *context = testcase->suite->refcon;
  struct run_under_result_store_store_context *store_context = context->store_context;
  struct run_under_result_store_suite_context *suite_context = context->suite_context;
  YR_ASSERT_FALSE(store_context->dummy1_seen_open);
  YR_ASSERT_FALSE(store_context->dummy1_seen_pass);
  YR_ASSERT_FALSE(store_context->dummy1_seen_close);
  YR_ASSERT_FALSE(store_context->dummy2_seen_open);
  YR_ASSERT_FALSE(store_context->dummy2_seen_pass);
  YR_ASSERT_FALSE(store_context->dummy2_seen_close);
  YR_ASSERT_FALSE(store_context->dummy3_seen_open);
  YR_ASSERT_FALSE(store_context->dummy3_seen_fail);
  YR_ASSERT_FALSE(store_context->dummy3_seen_close);

  YR_ASSERT_EQUAL(suite_context->rutabega[0], 0);
  YR_ASSERT_EQUAL(suite_context->rutabega[1], 0);
  YR_ASSERT_FALSE(suite_context->dummy1_seen_opened_hook);
  YR_ASSERT_FALSE(suite_context->dummy1_seen_case);
  YR_ASSERT_FALSE(suite_context->dummy1_seen_closed_hook);
  YR_ASSERT_FALSE(suite_context->dummy2_seen_opened_hook);
  YR_ASSERT_FALSE(suite_context->dummy2_seen_case);
  YR_ASSERT_FALSE(suite_context->dummy2_seen_closed_hook);
  YR_ASSERT_FALSE(suite_context->dummy3_seen_opened_hook);
  YR_ASSERT_FALSE(suite_context->dummy3_seen_case);
  YR_ASSERT_FALSE(suite_context->dummy3_seen_closed_hook);

  struct yr_result_callbacks result_callbacks = {0};
  yr_run_suite_under_store(context->suite, context->store, result_callbacks);

  YR_ASSERT(store_context->dummy1_seen_open);
  YR_ASSERT(store_context->dummy1_seen_pass);
  YR_ASSERT(store_context->dummy1_seen_close);
  YR_ASSERT(store_context->dummy2_seen_open);
  YR_ASSERT(store_context->dummy2_seen_pass);
  YR_ASSERT(store_context->dummy2_seen_close);
  YR_ASSERT(store_context->dummy3_seen_open);
  YR_ASSERT(store_context->dummy3_seen_fail);
  YR_ASSERT(store_context->dummy3_seen_close);

  YR_ASSERT_EQUAL(suite_context->rutabega[0], 0);
  YR_ASSERT_EQUAL(suite_context->rutabega[1], 0);
  YR_ASSERT(suite_context->dummy1_seen_opened_hook);
  YR_ASSERT(suite_context->dummy1_seen_case);
  YR_ASSERT(suite_context->dummy1_seen_closed_hook);
  YR_ASSERT(suite_context->dummy2_seen_opened_hook);
  YR_ASSERT(suite_context->dummy2_seen_case);
  YR_ASSERT(suite_context->dummy2_seen_closed_hook);
  YR_ASSERT(suite_context->dummy3_seen_opened_hook);
  YR_ASSERT(suite_context->dummy3_seen_case);
  YR_ASSERT(suite_context->dummy3_seen_closed_hook);

  YR_ASSERT_EQUAL(yr_result_store_get_result(context->store), YR_RESULT_FAILED);
}

static void _yr_enumerate_subresults_assert_closed(yr_result_store_t subresult, void *refcon)
{
  unsigned *visited = refcon;
  (*visited)++;
  YR_ASSERT(yr_result_store_is_closed(subresult));
  yr_result_store_enumerate(subresult, _yr_enumerate_subresults_assert_closed, refcon);
}
static YR_TESTCASE(test_run_under_store_closes_subresults_only)
{
  struct store_and_suite_context *context = testcase->suite->refcon;
  struct yr_result_callbacks result_callbacks = {0};
  yr_run_suite_under_store(context->suite, context->store, result_callbacks);
  YR_ASSERT_FALSE(yr_result_store_is_closed(context->store));
  unsigned visited = 0;
  yr_result_store_enumerate(context->store, _yr_enumerate_subresults_assert_closed, &visited);
  YR_ASSERT_EQUAL(visited, 4);
}

int main(void)
{
  struct yr_suite_lifecycle_callbacks suite_callbacks = {
    .setup_case = setup_store_and_suite,
    .teardown_case = teardown_store_and_suite
  };
  yr_test_suite_t suite = yr_create_suite_from_functions("run under store tests", NULL,
                                                         suite_callbacks,
                                                         test_run_under_store_callbacks,
                                                         test_run_under_store_closes_subresults_only);
  if ( yr_basic_run_suite(suite) ) {
    fprintf(stderr, "some tests failed!\n");
    return EXIT_FAILURE;
  }
  free(suite);
  return 0;
}

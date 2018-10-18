#include <yachtrock/yachtrock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct result_store_test_context
{
  yr_result_store_t store;
};
void result_store_case_setup(yr_test_case_t testcase)
{
  ((struct result_store_test_context *)(testcase->suite->refcon))->store =
    yr_result_store_create(testcase->name);
}
void result_store_case_teardown(yr_test_case_t testcase)
{
  struct result_store_test_context *test_context = (struct result_store_test_context *)(testcase->suite->refcon);
  yr_result_store_destroy(test_context->store);
  test_context->store = NULL;
}
#define GETSTORE(testcase) ((struct result_store_test_context *)(testcase)->suite->refcon)->store

void test_closing_store_passes(yr_test_case_t testcase)
{
  yr_result_store_t store = yr_result_store_create("fun things");
  YR_ASSERT(yr_result_store_get_result(store) == YR_RESULT_UNSET, "newly create should be unset");
  yr_result_store_close(store);
  YR_ASSERT(yr_result_store_get_result(store) == YR_RESULT_PASSED, "closed from unset should pass");
  yr_result_store_destroy(store);
}

void test_failing_subresult_fails_parent(yr_test_case_t testcase)
{
  yr_result_store_t store = yr_result_store_create("fun stuff!!");
  yr_result_store_t subresult = yr_result_store_open_subresult(store, "subtest");
  yr_result_store_record_result(subresult, YR_RESULT_FAILED);
  YR_ASSERT(yr_result_store_get_result(subresult) == YR_RESULT_FAILED);
  YR_ASSERT(yr_result_store_get_result(store) == YR_RESULT_FAILED);
  yr_result_store_close(store);
  yr_result_store_destroy(store);
}

void test_no_unfailing(yr_test_case_t testcase)
{
  yr_result_store_t store = yr_result_store_create("fun stuff!!");
  yr_result_store_record_result(store, YR_RESULT_FAILED);
  YR_ASSERT(yr_result_store_get_result(store) == YR_RESULT_FAILED);
  yr_result_store_record_result(store, YR_RESULT_PASSED);
  YR_ASSERT(yr_result_store_get_result(store) == YR_RESULT_FAILED);
  yr_result_store_destroy(store);
}

void test_unpassing(yr_test_case_t testcase)
{
  yr_result_store_t store = yr_result_store_create("fun stuff!!");
  yr_result_store_record_result(store, YR_RESULT_PASSED);
  YR_ASSERT(yr_result_store_get_result(store) == YR_RESULT_PASSED);
  yr_result_store_record_result(store, YR_RESULT_FAILED);
  YR_ASSERT(yr_result_store_get_result(store) == YR_RESULT_FAILED);
  yr_result_store_destroy(store);
}

void test_skips_basics(yr_test_case_t testcase)
{
  yr_result_store_t subresult = yr_result_store_open_subresult(GETSTORE(testcase), "subresult");
  yr_result_store_record_result(subresult, YR_RESULT_SKIPPED);
  yr_result_store_close(subresult);
  YR_ASSERT(yr_result_store_get_result(subresult) == YR_RESULT_SKIPPED);
  YR_ASSERT(yr_result_store_get_result(GETSTORE(testcase)) == YR_RESULT_UNSET);
  yr_result_store_close(GETSTORE(testcase));
  YR_ASSERT(yr_result_store_get_result(GETSTORE(testcase)) == YR_RESULT_PASSED);
}

void test_getters(yr_test_case_t testcase)
{
  yr_result_store_t subresult = yr_result_store_open_subresult(GETSTORE(testcase), "subresult");
  YR_ASSERT(yr_result_store_get_parent(subresult) == GETSTORE(testcase));
  YR_ASSERT(strcmp(yr_result_store_get_name(subresult), "subresult") == 0);
  YR_ASSERT(strcmp(yr_result_store_get_name(GETSTORE(testcase)), __FUNCTION__) == 0);
}

void enumerator_for_test(yr_result_store_t subresult, void *refcon)
{
  int context = (*(int *)refcon)++;
  char *expected_name = NULL;
  switch( context ) {
  case 0:
    yr_result_store_enumerate(subresult, enumerator_for_test, refcon);
    expected_name = "subresult";
    break;
  case 1:
    expected_name = "store1";
    break;
  case 2:
    expected_name = "store2";
    break;
  case 3:
    expected_name = "store3";
    break;
  }
  const char *realname = yr_result_store_get_name(subresult);
  YR_ASSERT(strcmp(realname, expected_name) == 0, "expected %s to be %s", realname, expected_name);
}
void test_enumeration(yr_test_case_t testcase)
{
  yr_result_store_t subresult = yr_result_store_open_subresult(GETSTORE(testcase), "subresult");
  yr_result_store_open_subresult(subresult, "store1");
  yr_result_store_open_subresult(subresult, "store2");
  yr_result_store_open_subresult(subresult, "store3");

  int context = 0;
  yr_result_store_enumerate(GETSTORE(testcase), enumerator_for_test, &context);
}

struct store_hooks_callbacks_refcon
{
  yr_result_store_t new_store;
  yr_result_store_t closed_store;
  yr_result_store_t changed_store;

  yr_result_store_t root;

  unsigned num_closes;
};

void test_store_opened_callback(yr_result_store_t new_store, void *refcon)
{
  ((struct store_hooks_callbacks_refcon *)refcon)->new_store = new_store;
}

void test_store_closed_callback(yr_result_store_t closed_store, void *refcon)
{
  if ( yr_result_store_get_parent(closed_store) != NULL ) {
    YR_ASSERT(strcmp(yr_result_store_get_name(closed_store), "child") == 0);
    YR_ASSERT(((struct store_hooks_callbacks_refcon *)refcon)->num_closes == 0);
    YR_ASSERT(yr_result_store_get_parent(closed_store) ==
              ((struct store_hooks_callbacks_refcon *)refcon)->root);
  } else {
    YR_ASSERT(closed_store == ((struct store_hooks_callbacks_refcon *)refcon)->root);
    YR_ASSERT(((struct store_hooks_callbacks_refcon *)refcon)->num_closes == 1);
  }
  ((struct store_hooks_callbacks_refcon *)refcon)->num_closes++;
  ((struct store_hooks_callbacks_refcon *)refcon)->closed_store = closed_store;
}

void test_store_result_changed_callback(yr_result_store_t changed_store, void *refcon)
{
  ((struct store_hooks_callbacks_refcon *)refcon)->changed_store = changed_store;
}

void test_hooks(void)
{
  struct store_hooks_callbacks_refcon refcon = {0};
  struct yr_result_hooks hooks;
  hooks.store_opened = test_store_opened_callback;
  hooks.store_closed = test_store_closed_callback;
  hooks.store_result_changed = test_store_result_changed_callback;

  yr_result_store_t store = yr_result_store_create_with_hooks("parent", hooks, &refcon);
  YR_ASSERT(refcon.new_store == store);
  YR_ASSERT(refcon.closed_store == NULL);
  YR_ASSERT(refcon.changed_store == NULL);

  refcon = (struct store_hooks_callbacks_refcon){0};

  yr_result_store_t child = yr_result_store_open_subresult(store, "child");
  YR_ASSERT(refcon.new_store == child);
  YR_ASSERT(refcon.closed_store == NULL);
  YR_ASSERT(refcon.changed_store == NULL);

  refcon = (struct store_hooks_callbacks_refcon){0};

  yr_result_store_record_result(child, YR_RESULT_PASSED);
  YR_ASSERT(refcon.new_store == NULL);
  YR_ASSERT(refcon.changed_store == child);
  YR_ASSERT(refcon.closed_store == NULL);

  refcon = (struct store_hooks_callbacks_refcon){0};

  yr_result_store_record_result(child, YR_RESULT_PASSED);
  YR_ASSERT(refcon.new_store == NULL);
  YR_ASSERT(refcon.changed_store == NULL);
  YR_ASSERT(refcon.closed_store == NULL);

  refcon = (struct store_hooks_callbacks_refcon){0};
  refcon.root = store;
  yr_result_store_close(store);
  YR_ASSERT(refcon.num_closes == 2);
  YR_ASSERT(refcon.closed_store == store);

  yr_result_store_destroy(store);
}

int main(void)
{
  struct result_store_test_context test_context;
  struct yr_suite_lifecycle_callbacks callbacks = {0};
  callbacks.setup_case = result_store_case_setup;
  callbacks.teardown_case = result_store_case_teardown;
  yr_test_suite_t suite = yr_create_suite_from_functions("result store tests", &callbacks,
                                                         test_closing_store_passes,
                                                         test_failing_subresult_fails_parent,
                                                         test_no_unfailing,
                                                         test_unpassing,
                                                         test_skips_basics,
                                                         test_getters,
                                                         test_enumeration,
                                                         test_hooks);
  suite->refcon = &test_context;
  int failures = yr_basic_run_suite(suite);
  if ( failures ) {
    fprintf(stderr, "some tests failed!\n");
    return EXIT_FAILURE;
  }
  free(suite);
  return 0;
}

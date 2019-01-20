#include <yachtrock/selector.h>
#include <yachtrock/testcase.h>
#include "yrtests.h"

static YR_TESTCASE(foobar) {}
static YR_TESTCASE(bazquux) {}

static int dummy;

static void setup_suite(yr_test_suite_t s) {}
static void setup_case(yr_test_case_t s) {}
static void teardown_case(yr_test_case_t s) {}
static void teardown_suite(yr_test_suite_t s) {}

struct yr_suite_lifecycle_callbacks dummy_lifecycle_callbacks = {
  .setup_suite = setup_suite,
  .setup_case = setup_case,
  .teardown_case = teardown_case,
  .teardown_suite = teardown_suite
};


static yr_test_suite_t create_suite_named(char *name)
{
  return yr_create_suite_from_functions(name, &dummy, dummy_lifecycle_callbacks, foobar, bazquux);
}

static YR_TESTCASE(test_degenerate_cases)
{
  yr_test_suite_t suite = create_suite_named("whatever");

  yr_selector_t selector = yr_selector_create_from_glob("");
  YR_ASSERT(yr_selector_match_testcase(selector, &suite->cases[0]));
  YR_ASSERT(yr_selector_match_testcase(selector, &suite->cases[1]));
  yr_selector_destroy(selector);

  selector = yr_selector_create_from_glob(":");
  YR_ASSERT(yr_selector_match_testcase(selector, &suite->cases[0]));
  YR_ASSERT(yr_selector_match_testcase(selector, &suite->cases[1]));
  yr_selector_destroy(selector);

  selector = yr_selector_create_from_glob("whatever:");
  YR_ASSERT(yr_selector_match_testcase(selector, &suite->cases[0]));
  YR_ASSERT(yr_selector_match_testcase(selector, &suite->cases[1]));
  yr_selector_destroy(selector);

  selector = yr_selector_create_from_glob("whateverf:");
  YR_ASSERT_FALSE(yr_selector_match_testcase(selector, &suite->cases[0]));
  YR_ASSERT_FALSE(yr_selector_match_testcase(selector, &suite->cases[1]));
  yr_selector_destroy(selector);

  selector = yr_selector_create_from_glob(":foobar");
  YR_ASSERT(yr_selector_match_testcase(selector, &suite->cases[0]));
  YR_ASSERT_FALSE(yr_selector_match_testcase(selector, &suite->cases[1]));
  yr_selector_destroy(selector);

  free(suite);

  suite = create_suite_named(":hello");

  selector = yr_selector_create_from_glob(":hello:");
  YR_ASSERT_FALSE(yr_selector_match_testcase(selector, &suite->cases[0]));
  YR_ASSERT_FALSE(yr_selector_match_testcase(selector, &suite->cases[1]));
  yr_selector_destroy(selector);

  selector = yr_selector_create_from_glob("\\:hello:");
  YR_ASSERT(yr_selector_match_testcase(selector, &suite->cases[0]));
  YR_ASSERT(yr_selector_match_testcase(selector, &suite->cases[1]));
  yr_selector_destroy(selector);

  free(suite);
}

static YR_TESTCASE(test_selector_from_string_glob_case_name)
{
  yr_test_suite_t suite = create_suite_named("whatever");

  yr_selector_t selector = yr_selector_create_from_glob("foobar");
  YR_ASSERT(yr_selector_match_testcase(selector, &suite->cases[0]));
  YR_ASSERT_FALSE(yr_selector_match_testcase(selector, &suite->cases[1]));
  yr_selector_destroy(selector);

  selector = yr_selector_create_from_glob("bazquux");
  YR_ASSERT(yr_selector_match_testcase(selector, &suite->cases[1]));
  YR_ASSERT_FALSE(yr_selector_match_testcase(selector, &suite->cases[0]));
  yr_selector_destroy(selector);

  selector = yr_selector_create_from_glob("*oob*");
  YR_ASSERT(yr_selector_match_testcase(selector, &suite->cases[0]));
  YR_ASSERT_FALSE(yr_selector_match_testcase(selector, &suite->cases[1]));
  yr_selector_destroy(selector);

  selector = yr_selector_create_from_glob("ba");
  YR_ASSERT_FALSE(yr_selector_match_testcase(selector, &suite->cases[0]));
  YR_ASSERT_FALSE(yr_selector_match_testcase(selector, &suite->cases[1]));
  yr_selector_destroy(selector);

  selector = yr_selector_create_from_glob("*ba*");
  YR_ASSERT(yr_selector_match_testcase(selector, &suite->cases[0]));
  YR_ASSERT(yr_selector_match_testcase(selector, &suite->cases[1]));
  yr_selector_destroy(selector);

  selector = yr_selector_create_from_glob("*ba??*");
  YR_ASSERT_FALSE(yr_selector_match_testcase(selector, &suite->cases[0]));
  YR_ASSERT(yr_selector_match_testcase(selector, &suite->cases[1]));
  yr_selector_destroy(selector);

  selector = yr_selector_create_from_glob("*whatever*");
  YR_ASSERT_FALSE(yr_selector_match_testcase(selector, &suite->cases[0]));
  YR_ASSERT_FALSE(yr_selector_match_testcase(selector, &suite->cases[1]));
  yr_selector_destroy(selector);

  free(suite);
}

static YR_TESTCASE(test_copy_selector)
{
  yr_test_suite_t suite = create_suite_named("whatever");
  yr_selector_t selector = yr_selector_create_from_glob("foobar");
  yr_selector_t copy = yr_selector_copy(selector);
  YR_ASSERT_NOT_EQUAL(copy, selector);
  YR_ASSERT_NOT_EQUAL(copy->context, selector->context);
  YR_ASSERT(yr_selector_match_testcase(selector, &suite->cases[0]));
  YR_ASSERT_FALSE(yr_selector_match_testcase(selector, &suite->cases[1]));
  yr_selector_destroy(selector);
  yr_selector_destroy(copy);
  free(suite);
}

static YR_TESTCASE(test_multiple_globs)
{
  yr_test_suite_t suite1 = create_suite_named("whatever");
  yr_test_suite_t suite2 = create_suite_named("you want");

  yr_selector_t selector = yr_selector_create_from_glob("*a*:foobar");
  YR_ASSERT(yr_selector_match_testcase(selector, &suite1->cases[0]));
  YR_ASSERT_FALSE(yr_selector_match_testcase(selector, &suite1->cases[1]));
  YR_ASSERT(yr_selector_match_testcase(selector, &suite2->cases[0]));
  YR_ASSERT_FALSE(yr_selector_match_testcase(selector, &suite2->cases[1]));
  yr_selector_destroy(selector);

  selector = yr_selector_create_from_glob("*hat*:foobar");
  YR_ASSERT(yr_selector_match_testcase(selector, &suite1->cases[0]));
  YR_ASSERT_FALSE(yr_selector_match_testcase(selector, &suite1->cases[1]));
  YR_ASSERT_FALSE(yr_selector_match_testcase(selector, &suite2->cases[0]));
  YR_ASSERT_FALSE(yr_selector_match_testcase(selector, &suite2->cases[1]));
  yr_selector_destroy(selector);

  selector = yr_selector_create_from_glob("*a*:*ba*");
  YR_ASSERT(yr_selector_match_testcase(selector, &suite1->cases[0]));
  YR_ASSERT(yr_selector_match_testcase(selector, &suite1->cases[1]));
  YR_ASSERT(yr_selector_match_testcase(selector, &suite2->cases[0]));
  YR_ASSERT(yr_selector_match_testcase(selector, &suite2->cases[1]));
  yr_selector_destroy(selector);

  free(suite1);
  free(suite2);
}

static bool match_only_string(yr_test_case_t testcase, void *context)
{
  char *str = context;
  return strcmp(testcase->name, str) == 0;
}

static void *copy_string_context(void *context)
{
  char *result = malloc(strlen(context) + 1);
  strcpy(result, context);
  return result;
}

static void destroy_string_context(void *context)
{
  free(context);
}

static YR_TESTCASE(rutabega) {}

static YR_TESTCASE(test_selector_raw_api)
{
  struct yr_selector_vtable vtable = {
    .match = match_only_string,
    .copy_context = copy_string_context,
    .destroy_context = destroy_string_context
  };
  struct yr_selector sel_start = {
    .vtable = vtable,
    .context = "rutabega"
  };
  yr_selector_t copied = yr_selector_copy(&sel_start);
  YR_ASSERT_EQUAL(copied->vtable.match, sel_start.vtable.match);
  YR_ASSERT_EQUAL(copied->vtable.copy_context, sel_start.vtable.copy_context);
  YR_ASSERT_EQUAL(copied->vtable.destroy_context, sel_start.vtable.destroy_context);
  YR_ASSERT_NOT_EQUAL(copied->context, sel_start.context);
  YR_ASSERT_EQUAL(strcmp(copied->context, sel_start.context), 0);

  yr_test_suite_t suite = yr_create_suite_from_functions("boo", NULL, YR_NO_CALLBACKS,
                                                         foobar, bazquux, rutabega);
  YR_ASSERT_FALSE(yr_selector_match_testcase(copied, &suite->cases[0]));
  YR_ASSERT_FALSE(yr_selector_match_testcase(copied, &suite->cases[1]));
  YR_ASSERT(yr_selector_match_testcase(copied, &suite->cases[2]));
  
  yr_selector_destroy(copied);
  free(suite);
}

static YR_TESTCASE(test_selector_sets_no_match)
{
  yr_test_suite_t suite = create_suite_named("whatever");
  yr_selector_t sel1 = yr_selector_create_from_glob("blah");
  yr_selector_t sel2 = yr_selector_create_from_glob("blah2");
  yr_selector_t selectors[] = {sel1, sel2};
  yr_selector_set_t set = yr_selector_set_create(2, selectors);
  yr_selector_destroy(sel1);
  yr_selector_destroy(sel2);
  YR_ASSERT_FALSE(yr_selector_set_match_testcase(set, &suite->cases[0]));
  YR_ASSERT_FALSE(yr_selector_set_match_testcase(set, &suite->cases[1]));
  yr_selector_set_destroy(set);
  free(suite);
}

static YR_TESTCASE(test_selector_sets_one_match)
{
  yr_test_suite_t suite = create_suite_named("whatever");
  yr_selector_t sel1 = yr_selector_create_from_glob("blah");
  yr_selector_t sel2 = yr_selector_create_from_glob("foo*");
  yr_selector_t selectors[] = {sel1, sel2};
  yr_selector_set_t set = yr_selector_set_create(2, selectors);
  yr_selector_destroy(sel1);
  yr_selector_destroy(sel2);
  YR_ASSERT(yr_selector_set_match_testcase(set, &suite->cases[0]));
  YR_ASSERT_FALSE(yr_selector_set_match_testcase(set, &suite->cases[1]));
  yr_selector_set_destroy(set);
  free(suite);
}

static YR_TESTCASE(test_selector_sets_all_match)
{
  yr_test_suite_t suite = create_suite_named("whatever");
  yr_selector_t sel1 = yr_selector_create_from_glob("*oo*");
  yr_selector_t sel2 = yr_selector_create_from_glob("foo*");
  yr_selector_t selectors[] = {sel1, sel2};
  yr_selector_set_t set = yr_selector_set_create(2, selectors);
  yr_selector_destroy(sel1);
  yr_selector_destroy(sel2);
  YR_ASSERT(yr_selector_set_match_testcase(set, &suite->cases[0]));
  YR_ASSERT_FALSE(yr_selector_set_match_testcase(set, &suite->cases[1]));
  yr_selector_set_destroy(set);
  free(suite);
}

static bool lifecycles_equal(struct yr_suite_lifecycle_callbacks callbacks1,
                             struct yr_suite_lifecycle_callbacks callbacks2)
{
  return (callbacks1.setup_suite == callbacks2.setup_suite &&
          callbacks1.setup_case == callbacks2.setup_case &&
          callbacks1.teardown_case == callbacks2.teardown_case &&
          callbacks1.teardown_suite == callbacks2.teardown_suite);
}

static YR_TESTCASE(test_suite_filtering_some_hits)
{
  yr_test_suite_t suite = create_suite_named("whatever");
  yr_selector_t sel1 = yr_selector_create_from_glob("*az*");
  yr_selector_set_t set = yr_selector_set_create(1, &sel1);
  yr_selector_destroy(sel1);
  yr_test_suite_t filtered = yr_test_suite_create_filtered(suite, set);
  YR_ASSERT_EQUAL(filtered->num_cases, 1);
  YR_ASSERT(strcmp(filtered->cases[0].name, suite->cases[1].name) == 0);
  YR_ASSERT_EQUAL(filtered->cases[0].testcase, suite->cases[1].testcase);
  YR_ASSERT_NOT_EQUAL(filtered->cases[0].name, suite->cases[1].name);
  YR_ASSERT_EQUAL(filtered->refcon, suite->refcon);
  YR_ASSERT(lifecycles_equal(filtered->lifecycle, suite->lifecycle));

  free(filtered);
  free(suite);
  yr_selector_set_destroy(set);
}

static YR_TESTCASE(test_suite_filtering_no_hits)
{
  yr_test_suite_t suite = create_suite_named("whatever");
  yr_selector_t sel1 = yr_selector_create_from_glob("garg");
  yr_selector_set_t set = yr_selector_set_create(1, &sel1);
  yr_selector_destroy(sel1);
  yr_test_suite_t filtered = yr_test_suite_create_filtered(suite, set);
  YR_ASSERT_EQUAL(filtered, NULL);
  free(suite);
  yr_selector_set_destroy(set);
}

static yr_test_suite_collection_t create_testing_collection(void)
{
  yr_test_suite_t suite1 = create_suite_named("whatever");
  yr_test_suite_t suite2 = yr_create_blank_suite(3);
  suite2->name = "helo";
  suite2->refcon = (char *)suite2->name;
  suite2->lifecycle = dummy_lifecycle_callbacks;
  suite2->cases[0].testcase = foobar;
  suite2->cases[0].name = "glorp";
  suite2->cases[1].testcase = foobar;
  suite2->cases[1].name = "beep";
  suite2->cases[2].testcase = foobar;
  suite2->cases[2].name = "toot";
  yr_test_suite_t suite3 = yr_create_blank_suite(3);
  suite3->name = "final";
  suite3->refcon = (char *)suite2->name;
  suite3->lifecycle = dummy_lifecycle_callbacks;
  suite3->cases[0].testcase = foobar;
  suite3->cases[0].name = "glorp";
  suite3->cases[1].testcase = foobar;
  suite3->cases[1].name = "beep";
  suite3->cases[2].testcase = foobar;
  suite3->cases[2].name = "toot";

  yr_test_suite_t suites[] = {suite1, suite2, suite3};
  yr_test_suite_collection_t collection = yr_test_suite_collection_create_from_suites(3, suites);
  free(suite1);
  free(suite2);
  free(suite3);
  return collection;
}
static YR_TESTCASE(test_collection_filtering_some_hits)
{
  yr_test_suite_collection_t collection = create_testing_collection();
  yr_selector_t sel1 = yr_selector_create_from_glob("*az*");
  yr_selector_t sel2 = yr_selector_create_from_glob("*nal:");
  yr_selector_t sels[] = {sel1, sel2};
  yr_selector_set_t set = yr_selector_set_create(2, sels);
  yr_selector_destroy(sel1);
  yr_selector_destroy(sel2);
  yr_test_suite_collection_t filtered = yr_test_suite_collection_create_filtered(collection, set);

  YR_ASSERT_EQUAL(filtered->num_suites, 2);

  YR_ASSERT(strcmp(filtered->suites[0]->name, collection->suites[0]->name) == 0);
  YR_ASSERT(lifecycles_equal(filtered->suites[0]->lifecycle, collection->suites[0]->lifecycle));
  YR_ASSERT_EQUAL(filtered->suites[0]->num_cases, 1);
  YR_ASSERT_EQUAL(filtered->suites[0]->cases[0].testcase, bazquux);
  YR_ASSERT(strcmp(filtered->suites[0]->cases[0].name, "bazquux") == 0);

  YR_ASSERT(strcmp(filtered->suites[1]->name, collection->suites[2]->name) == 0);
  YR_ASSERT(lifecycles_equal(filtered->suites[1]->lifecycle, collection->suites[2]->lifecycle));
  YR_ASSERT_EQUAL(filtered->suites[1]->num_cases, 3);
  YR_ASSERT_EQUAL(filtered->suites[1]->cases[0].testcase, foobar);
  YR_ASSERT(strcmp(filtered->suites[1]->cases[0].name, collection->suites[2]->cases[0].name) == 0);
  YR_ASSERT_NOT_EQUAL(filtered->suites[1]->cases[0].name, collection->suites[2]->cases[0].name);
  YR_ASSERT_EQUAL(filtered->suites[1]->cases[1].testcase, foobar);
  YR_ASSERT(strcmp(filtered->suites[1]->cases[1].name, collection->suites[2]->cases[1].name) == 0);
  YR_ASSERT_NOT_EQUAL(filtered->suites[1]->cases[1].name, collection->suites[2]->cases[1].name);
  YR_ASSERT_EQUAL(filtered->suites[1]->cases[2].testcase, foobar);
  YR_ASSERT(strcmp(filtered->suites[1]->cases[2].name, collection->suites[2]->cases[2].name) == 0);
  YR_ASSERT_NOT_EQUAL(filtered->suites[1]->cases[2].name, collection->suites[2]->cases[2].name);

  yr_selector_set_destroy(set);
  free(collection);
  free(filtered);
}

static YR_TESTCASE(test_collection_filtering_no_hits)
{
  yr_test_suite_collection_t collection = create_testing_collection();
  yr_selector_t sel1 = yr_selector_create_from_glob("*blah*");
  yr_selector_t sel2 = yr_selector_create_from_glob("grr:");
  yr_selector_t sels[] = {sel1, sel2};
  yr_selector_set_t set = yr_selector_set_create(2, sels);
  yr_selector_destroy(sel1);
  yr_selector_destroy(sel2);
  yr_test_suite_collection_t filtered = yr_test_suite_collection_create_filtered(collection, set);
  YR_ASSERT_EQUAL(filtered, NULL);
  yr_selector_set_destroy(set);
  free(collection);
}

yr_test_suite_t yr_create_selector_suite(void)
{
  return yr_create_suite_from_functions("selector tests", NULL, YR_NO_CALLBACKS,
                                        test_degenerate_cases,
                                        test_selector_from_string_glob_case_name,
                                        test_copy_selector,
                                        test_multiple_globs,
                                        test_selector_raw_api,
                                        test_selector_sets_no_match,
                                        test_selector_sets_one_match,
                                        test_selector_sets_all_match,
                                        test_suite_filtering_some_hits,
                                        test_suite_filtering_no_hits,
                                        test_collection_filtering_some_hits,
                                        test_collection_filtering_no_hits);
}

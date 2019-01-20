#include <yachtrock/selector.h>

#include <fnmatch.h>
#include <assert.h>

#include "yrutil.h"
#include "testcase_internal.h"

struct parsed_glob_specifier {
  char *suite_glob;
  char *case_glob;
  char storage[];
};

static bool match_glob(const char *glob, const char *input)
{
  if ( glob == NULL ) {
    return true;
  } else if ( input == NULL ) {
    return false;
  }

  return fnmatch(glob, input, 0) == 0;
}

static bool testname_glob_match(yr_test_case_t testcase, void *context)
{
  struct parsed_glob_specifier *parsed = context;
  return match_glob(parsed->suite_glob, testcase->suite->name) &&
    match_glob(parsed->case_glob, testcase->name);
}

static struct parsed_glob_specifier *create_parsed_glob_specifier(char *suite, char *testcase)
{
  size_t string_size = (suite ? strlen(suite) + 1 : 0) + (testcase ? strlen(testcase) + 1 : 0);
  struct parsed_glob_specifier *result =
    yr_malloc(sizeof(struct parsed_glob_specifier) + string_size);
  char *insertion_point = result->storage;
  if ( suite ) {
    result->suite_glob = insertion_point;
    strcpy(insertion_point, suite);
    insertion_point += strlen(insertion_point) + 1;
  } else {
    result->suite_glob = NULL;
  }
  if ( testcase ) {
    result->case_glob = insertion_point;
    strcpy(insertion_point, testcase);
    insertion_point += strlen(insertion_point) + 1;
  } else {
    result ->case_glob = NULL;
  }

  assert(insertion_point <= result->storage + string_size);
  return result;
}

static void *testname_glob_context_copy(void *context)
{
  struct parsed_glob_specifier *parsed = context;
  struct parsed_glob_specifier *copy = create_parsed_glob_specifier(parsed->suite_glob,
                                                                    parsed->case_glob);
  return copy;
}

static void testname_glob_context_destroy(void *context)
{
  free(context);
}

static struct yr_selector_vtable testname_glob_selector_vtable = {
  .match = testname_glob_match,
  .copy_context = testname_glob_context_copy,
  .destroy_context = testname_glob_context_destroy,
};

yr_selector_t yr_selector_create_from_glob(const char *sel_specifier)
{
  /* Right now name globbing is the only thing that exists.
   * split at the first unescaped ':'; before is suite glob, after is case glob.
   * If no separator exists, treat as a case glob with no suite glob.
   * If either glob is the empty string, treat it as not existing (so ":" always matches)
   */
  char specifier_copy[strlen(sel_specifier) + 1];
  strcpy(specifier_copy, sel_specifier);
  char *suite=NULL, *testcase=NULL;
  for ( size_t i = 0; specifier_copy[i]; i++ ) {
    if ( specifier_copy[i] == '\\' && specifier_copy[i+1] == ':' ) {
      // skip the slash; subsequently skip the escaped ':'
      i++;
      continue;
    } else if ( specifier_copy[i] == ':' ) {
      specifier_copy[i] = '\0';
      suite = specifier_copy;
      testcase = specifier_copy + i + 1;
      break;
    }
  }
  /* At this point if both glob pointers are still NULL, we did not find an unescaped ':'; set
   * testcase glob
   */
  if ( suite == NULL && testcase == NULL ) {
    testcase = specifier_copy;
  }
  if ( suite && strlen(suite) == 0 ) {
    suite = NULL;
  }
  if ( testcase && strlen(testcase) == 0 ) {
    testcase = NULL;
  }
  struct parsed_glob_specifier *parsed = create_parsed_glob_specifier(suite, testcase);
  yr_selector_t result = yr_malloc(sizeof(struct yr_selector));
  result->vtable = testname_glob_selector_vtable;
  result->context = parsed;
  return result;
}

void yr_selector_destroy(yr_selector_t selector)
{
  selector->vtable.destroy_context(selector->context);
  free(selector);
}

yr_selector_t yr_selector_copy(yr_selector_t in)
{
  yr_selector_t selector = yr_malloc(sizeof(struct yr_selector));
  selector->vtable = in->vtable;
  selector->context = in->vtable.copy_context(in->context);
  return selector;
}

bool yr_selector_match_testcase(yr_selector_t selector, yr_test_case_t testcase)
{
  return selector->vtable.match(testcase, selector->context);
}

yr_selector_set_t yr_selector_set_create(size_t selector_count,
                                              yr_selector_t *selectors)
{
  yr_selector_set_t result = yr_malloc(sizeof(struct yr_selector_set) +
                                            selector_count * sizeof(struct yr_selector));
  result->selector_count = selector_count;
  for ( size_t i = 0; i < selector_count; i++ ) {
    result->selectors[i].vtable = selectors[i]->vtable;
    result->selectors[i].context = selectors[i]->vtable.copy_context(selectors[i]->context);
  }
  return result;
}

void yr_selector_set_destroy(yr_selector_set_t set)
{
  for ( size_t i = 0; i < set->selector_count; i++ ) {
    set->selectors[i].vtable.destroy_context(set->selectors[i].context);
  }
  free(set);
}

bool yr_selector_set_match_testcase(yr_selector_set_t set,
                                    yr_test_case_t testcase)
{
  for ( size_t i = 0; i < set->selector_count; i++ ) {
    if ( yr_selector_match_testcase(&(set->selectors[i]), testcase) ) {
      return true;
    }
  }
  return false;
}

static char *dup_string_bumping_string_area(const char *input, char **area_ptr)
{
  strcpy(*area_ptr, input);
  char *result = *area_ptr;
  *area_ptr += strlen(*area_ptr) + 1;
  return result;
}

yr_test_suite_t yr_test_suite_create_filtered(yr_test_suite_t suite, yr_selector_set_t filter)
{
  yr_test_suite_t result = NULL;
  yr_test_case_t *cases = yr_malloc(sizeof(yr_test_case_t) * suite->num_cases);
  size_t num_filtered_cases = 0;
  for ( size_t i = 0; i < suite->num_cases; i++ ) {
    if ( yr_selector_set_match_testcase(filter, &suite->cases[i]) ) {
      cases[num_filtered_cases++] = &suite->cases[i];
    }
  }

  if ( num_filtered_cases != 0 ) {
    char *names[num_filtered_cases];
    for ( size_t i = 0; i < num_filtered_cases; i++ ) {
      /* need to cast away const here; we promise not to munge them */
      names[i] = (char *)cases[i]->name;
    }
    size_t allocation_size = test_suite_total_size_deconstructed(suite->name, num_filtered_cases,
                                                                 names);
    result = yr_malloc(allocation_size);

    /* Copy everything, strings always last
     * (Need to set num cases before calculating string storage location)
     */
    *result = *suite;
    result->num_cases = num_filtered_cases;
    char *string_insertion_point = test_suite_string_storage_location(result);
    result->name = dup_string_bumping_string_area(suite->name, &string_insertion_point);
    for ( size_t i = 0; i < num_filtered_cases; i++ ) {
      result->cases[i] = *cases[i];
      result->cases[i].suite = result;
      result->cases[i].name = dup_string_bumping_string_area(cases[i]->name, &string_insertion_point);
    }

    assert(string_insertion_point - ((char *)result) <= allocation_size);
  }

  free(cases);
  return result;
}

yr_test_suite_collection_t
yr_test_suite_collection_create_filtered(yr_test_suite_collection_t collection,
                                         yr_selector_set_t filter)
{
  yr_test_suite_collection_t result = NULL;
  yr_test_suite_t *suites = yr_malloc(sizeof(yr_test_suite_t) * collection->num_suites);
  size_t num_suites = 0;
  for ( size_t i = 0; i < collection->num_suites; i++ ) {
    yr_test_suite_t filtered = yr_test_suite_create_filtered(collection->suites[i], filter);
    if ( filtered ) {
      suites[num_suites++] = filtered;
    }
  }

  if ( num_suites > 0 ) {
    result = yr_test_suite_collection_create_from_suites(num_suites, suites);
  }

  for ( size_t i = 0; i < num_suites; i++ ) {
    free(suites[i]);
  }
  free(suites);

  return result;
}

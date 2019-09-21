#ifndef YACHTROCK_SELECTOR_H
#define YACHTROCK_SELECTOR_H

#include <yachtrock/testcase.h>
#include <stdbool.h>

/**
 * The vtable that defines selector behavior.
 *
 * match: returns true if the testcase matches the selector's criteria.
 * copy_context: copies and returns the selector-specific context.
 * destroy_context: releases resources held by the selector-specific context.
 */
struct yr_selector_vtable {
  bool (*match)(yr_test_case_t testcase, void *context);
  void *(*copy_context)(void *context);
  void (*destroy_context)(void *context);
};

/**
 * A testcase selector.
 */
typedef struct yr_selector {
  struct yr_selector_vtable vtable;
  void *context;
} *yr_selector_t;

/**
 * A set of testcase selectors.
 */
typedef struct yr_selector_set {
  size_t selector_count;
  struct yr_selector selectors[];
} *yr_selector_set_t;

/**
 * Create a "glob" testcase selector.
 *
 * This takes a specifier of the form [<suite_glob>:]<case_glob>, with both globs having the meaning
 * ascribed by fnmatch(3).
 */
YACHTROCK_EXTERN yr_selector_t yr_selector_create_from_glob(const char *sel_specifier);

/**
 * Create a selector with the given vtable and context.
 *
 * The context is used directly, that is, it is not first copied.
 */
YACHTROCK_EXTERN yr_selector_t yr_selector_create(struct yr_selector_vtable vtable, void *context);

/**
 * Copy a selector.
 *
 * A new selector structure is allocated and its vtable and copied context are assigned.
 */
YACHTROCK_EXTERN yr_selector_t yr_selector_copy(yr_selector_t in);

/**
 * Destroy a selector.
 *
 * The selector is freed after the context destroy method is applied to its context.
 */
YACHTROCK_EXTERN void yr_selector_destroy(yr_selector_t selector);

/**
 * Returns true if the selector matches the testcase.
 */
YACHTROCK_EXTERN bool yr_selector_match_testcase(yr_selector_t selector,
                                                 yr_test_case_t testcase);


/**
 * Create a selector set from an array of selectors.
 */
YACHTROCK_EXTERN yr_selector_set_t yr_selector_set_create(size_t selector_count,
                                                          yr_selector_t *selectors);

/**
 * Destroy a selector set.
 */
YACHTROCK_EXTERN void yr_selector_set_destroy(yr_selector_set_t set);

/**
 * Determine if a selector set matches a testcase.
 *
 * This is true if any one of the selectors in the set match the testcase.
 */
YACHTROCK_EXTERN bool yr_selector_set_match_testcase(yr_selector_set_t set,
                                                     yr_test_case_t testcase);

/**
 * Filter a test suite with a selector set.
 *
 * The resulting test suite contains only cases that match the selector set. It is also a single
 * allocation, so all resources of the filtered set can and must be freed by the caller via a call
 * to free(3), providing the suite.
 *
 * This function returns NULL, rather than an empty suite, if all cases were filtered out.
 */
YACHTROCK_EXTERN yr_test_suite_t
yr_test_suite_create_filtered(yr_test_suite_t suite, yr_selector_set_t filter);

/**
 * Create a test suite collection from the input collection subject to the selector set's filter.
 *
 * The resulting suite collection is a single allocation and must be freed by the caller.
 *
 * This function returns NULL, rather than an empty collection, if all cases were filtered out.
 */
YACHTROCK_EXTERN yr_test_suite_collection_t
yr_test_suite_collection_create_filtered(yr_test_suite_collection_t collection,
                                         yr_selector_set_t filter);

#endif

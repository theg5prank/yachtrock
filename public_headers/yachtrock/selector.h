#ifndef YACHTROCK_SELECTOR_H
#define YACHTROCK_SELECTOR_H

#include <yachtrock/testcase.h>
#include <stdbool.h>
#include <stddef.h>

struct yr_selector_vtable {
  bool (*match)(yr_test_case_t testcase, void *context);
  void *(*copy_context)(void *context);
  void (*destroy_context)(void *context);
};

typedef struct yr_selector {
  struct yr_selector_vtable vtable;
  void *context;
} *yr_selector_t;

typedef struct yr_selector_set {
  size_t selector_count;
  struct yr_selector selectors[];
} *yr_selector_set_t;

YACHTROCK_EXTERN yr_selector_t yr_selector_create_from_glob(const char *sel_specifier);
YACHTROCK_EXTERN yr_selector_t yr_selector_copy(yr_selector_t in);
YACHTROCK_EXTERN void yr_selector_destroy(yr_selector_t selector);
YACHTROCK_EXTERN bool yr_selector_match_testcase(yr_selector_t selector,
                                                 yr_test_case_t testcase);

YACHTROCK_EXTERN yr_selector_set_t yr_selector_set_create(size_t selector_count,
                                                               yr_selector_t *selectors);
YACHTROCK_EXTERN void yr_selector_set_destroy(yr_selector_set_t set);
YACHTROCK_EXTERN bool yr_selector_set_match_testcase(yr_selector_set_t set,
                                                     yr_test_case_t testcase);

YACHTROCK_EXTERN yr_test_suite_t
yr_test_suite_create_filtered(yr_test_suite_t suite, yr_selector_set_t filter);

YACHTROCK_EXTERN yr_test_suite_collection_t
yr_test_suite_collection_create_filtered(yr_test_suite_collection_t collection,
                                         yr_selector_set_t filter);

#endif

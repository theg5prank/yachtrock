#ifndef YACHTROCK_RESULTS_H
#define YACHTROCK_RESULTS_H

#include <yachtrock/base.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum yr_result {
  YR_RESULT_UNSET,
  YR_RESULT_PASSED,
  YR_RESULT_FAILED,
  YR_RESULT_SKIPPED
} yr_result_t;

YACHTROCK_EXTERN yr_result_t yr_merge_result(yr_result_t old, yr_result_t new);

typedef struct yr_result_store yr_result_store_s;
typedef yr_result_store_s *yr_result_store_t;

struct yr_result_hooks {
  void *context;
  void (*store_opened)(yr_result_store_t new_store, void *refcon);
  void (*store_closed)(yr_result_store_t closed_store, void *refcon);
  void (*store_result_changed)(yr_result_store_t store, void *refcon);
};

YACHTROCK_EXTERN yr_result_store_t yr_result_store_create(const char *name);
YACHTROCK_EXTERN yr_result_store_t yr_result_store_create_with_hooks(const char *name,
                                                                     struct yr_result_hooks hooks);
YACHTROCK_EXTERN void yr_result_store_destroy(yr_result_store_t store);

YACHTROCK_EXTERN void yr_result_store_close(yr_result_store_t store);
YACHTROCK_EXTERN bool yr_result_store_is_closed(yr_result_store_t store);
YACHTROCK_EXTERN yr_result_store_t yr_result_store_open_subresult(yr_result_store_t store, const char *name);
YACHTROCK_EXTERN void yr_result_store_record_result(yr_result_store_t store, yr_result_t result);
YACHTROCK_EXTERN yr_result_t yr_result_store_get_result(yr_result_store_t store);
YACHTROCK_EXTERN const char *yr_result_store_get_name(yr_result_store_t store);
YACHTROCK_EXTERN yr_result_store_t yr_result_store_get_parent(yr_result_store_t store);

typedef void (*yr_result_store_enumerator_t)(yr_result_store_t subresult, void *refcon);
YACHTROCK_EXTERN void yr_result_store_enumerate(yr_result_store_t store,
                                                yr_result_store_enumerator_t enumerator, void *refcon);
YACHTROCK_EXTERN size_t yr_result_store_count_subresults(yr_result_store_t store);
YACHTROCK_EXTERN yr_result_store_t yr_result_store_get_subresult(yr_result_store_t store,
                                                                 size_t i);

YACHTROCK_EXTERN size_t yr_result_store_get_description(yr_result_store_t store, char *buf, size_t buf_size);
YACHTROCK_EXTERN char *yr_result_store_copy_description(yr_result_store_t store);

#endif

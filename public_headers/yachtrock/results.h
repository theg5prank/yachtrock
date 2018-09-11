#ifndef YACHTROCK_RESULTS_H
#define YACHTROCK_RESULTS_H

#include <yachtrock/base.h>

typedef enum yr_result {
  YR_RESULT_UNSET,
  YR_RESULT_PASSED,
  YR_RESULT_FAILED,
  YR_RESULT_SKIPPED
} yr_result_t;

typedef struct yr_result_store yr_result_store_s;
typedef yr_result_store_s *yr_result_store_t;

YACHTROCK_EXTERN yr_result_store_t yr_result_store_create(void);
YACHTROCK_EXTERN void yr_result_store_destroy(yr_result_store_t store);

YACHTROCK_EXTERN yr_result_store_t yr_result_store_open_subresult(yr_result_store_t store, char *name);
YACHTROCK_EXTERN void yr_result_store_record_result(yr_result_store_t store, yr_result_t result);
YACHTROCK_EXTERN yr_result_t yr_result_store_get_result(yr_result_store_t store);

typedef void (*yr_result_store_enumerator_t)(yr_result_store_t subresult, const char *name, void *refcon);
YACHTROCK_EXTERN void yr_result_store_enumerate(yr_result_store_enumerator_t enumerator, void *refcon);

#endif

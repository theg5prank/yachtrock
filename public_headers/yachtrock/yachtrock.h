#ifndef YACHTROCK_YACHTROCK_H
#define YACHTROCK_YACHTROCK_H

#include <yachtrock/base.h>
#include <yachtrock/assert.h>
#include <yachtrock/testcase.h>
#include <yachtrock/results.h>

YACHTROCK_EXTERN struct yr_runtime_callbacks YR_BASIC_STDERR_RUNTIME_CALLBACKS;
YACHTROCK_EXTERN struct yr_result_hooks YR_BASIC_STDERR_RESULT_HOOKS;

YACHTROCK_EXTERN int yr_basic_run_suite(yr_test_suite_t suite);
YACHTROCK_EXTERN int yr_run_suite_with_result_hooks(yr_test_suite_t suite,
                                                    struct yr_result_hooks hooks,
                                                    struct yr_runtime_callbacks runtime_callbacks);

YACHTROCK_EXTERN void yr_run_suite_under_store(yr_test_suite_t suite,
                                               yr_result_store_t store,
                                               struct yr_runtime_callbacks runtime_callbacks);
YACHTROCK_EXTERN void yr_run_suite_collection_under_store(yr_test_suite_collection_t collection,
                                                          yr_result_store_t store,
                                                          struct yr_runtime_callbacks runtime_callbacks);

#endif

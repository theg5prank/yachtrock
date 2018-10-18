#ifndef YACHTROCK_YACHTROCK_H
#define YACHTROCK_YACHTROCK_H

#include <yachtrock/base.h>
#include <yachtrock/assert.h>
#include <yachtrock/testcase.h>
#include <yachtrock/results.h>

YACHTROCK_EXTERN int yr_basic_run_suite(yr_test_suite_t suite);
YACHTROCK_EXTERN int yr_run_suite_with_result_hooks(yr_test_suite_t suite,
                                                    struct yr_result_hooks hooks,
                                                    void *result_hook_refcon,
                                                    struct yr_result_callbacks result_callbacks);

#endif

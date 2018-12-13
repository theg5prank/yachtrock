#ifndef YACHTROCK_YACHTROCK_H
#define YACHTROCK_YACHTROCK_H

#include <stdbool.h>

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

#if YACHTROCK_POSIXY

/**
 * Run test code in an inferior process.
 *
 * All test cases along with their attendant lifecycle callbacks are run in the inferior process.
 * The runtime callbacks are also run in the inferior process.
 *
 * Result store hooks are run in the original "parent" process and not in the inferior process.
 *
 * The inferior process is launched by spawning with the given arguments and environment, though the
 * implementation may modify the environment. When the inferior process first invokes
 * yr_run_suite_collection_under_store_multiprocess, it will attempt to rendezvous with the original
 * "parent" process and begin testing. This is done so that the responsibility of loading the test code
 * etc remains on client code. If you don't generate your testcases randomly and do a bunch of work prior
 * to calling into libyachtrock, a good approach is to simply provide your own argc and argv.
 *
 * The implementation performs some sanity checking to make sure the test suite collections in both
 * processes appear to be the "same" before the inferior process is instructed to begin testing.
 * Currently "same" just means that there are the same number of suites, with the same names, each of which
 * contain the same number of cases, with the same names.
 *
 * Note that the inferior process does read the environment through the getenv function.
 */
YACHTROCK_EXTERN void
yr_run_suite_collection_under_store_multiprocess(char *path, char **argv, char **environ,
                                                 yr_test_suite_collection_t collection,
                                                 yr_result_store_t store,
                                                 struct yr_runtime_callbacks runtime_callbacks);

YACHTROCK_EXTERN bool
yr_process_is_inferior(void);

#endif

#endif

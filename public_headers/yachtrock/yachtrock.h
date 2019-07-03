#ifndef YACHTROCK_YACHTROCK_H
#define YACHTROCK_YACHTROCK_H

#include <stdbool.h>
#include <stdnoreturn.h>

#include <yachtrock/base.h>
#include <yachtrock/assert.h>
#include <yachtrock/testcase.h>
#include <yachtrock/results.h>
#include <yachtrock/selector.h>

YACHTROCK_EXTERN const struct yr_runtime_callbacks YR_BASIC_STDERR_RUNTIME_CALLBACKS;
YACHTROCK_EXTERN const struct yr_result_hooks YR_BASIC_STDERR_RESULT_HOOKS;

YACHTROCK_EXTERN bool yr_basic_run_suite(yr_test_suite_t suite);
YACHTROCK_EXTERN bool yr_run_suite_with_result_hooks(yr_test_suite_t suite,
                                                     struct yr_result_hooks hooks,
                                                     struct yr_runtime_callbacks runtime_callbacks);

YACHTROCK_EXTERN void yr_run_suite_under_store(yr_test_suite_t suite,
                                               yr_result_store_t store,
                                               struct yr_runtime_callbacks runtime_callbacks);
YACHTROCK_EXTERN void yr_run_suite_collection_under_store(yr_test_suite_collection_t collection,
                                                          yr_result_store_t store,
                                                          struct yr_runtime_callbacks runtime_callbacks);

#if YACHTROCK_MULTIPROCESS

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

/**
 * Determine whether the process is the inferior process.
 */
YACHTROCK_EXTERN bool yr_process_is_inferior(void);

/**
 * Assuming the process is the inferior process, check in with the waiting superior process.
 *
 * This function may be easier to use in some cases than just calling into
 * yr_run_suite_collection_under_store_multiprocess in the inferior as normal; however, it will
 * abort if the process is not the inferior.
 */
YACHTROCK_EXTERN noreturn void yr_inferior_checkin(yr_test_suite_collection_t collection,
                                                   struct yr_runtime_callbacks runtime_callbacks);

/**
 * If the process is an inferior, stop being an inferior.
 *
 * This means, for instance, that functions like yr_process_is_inferior will return false and the
 * process will be a superior process when calling yr_run_suite_collection_under_store_multiprocess.
 *
 * This can be undone by calling yr_reset_inferiority. The calls increment and decrement a counter,
 * so if this function is called multiple times, yr_reset_inferiority must be called that same
 * number of times to return the process to its initial state. If the process was never an inferior,
 * this function (and yr_reset_inferiority) does nothing.
 *
 * This is probably not a very useful function unless you are testing libyachtrock.
 */
YACHTROCK_EXTERN void yr_suspend_inferiority(void);

/**
 * Undo a call to yr_suspend_inferiority.
 */
YACHTROCK_EXTERN void yr_reset_inferiority(void);

#endif

#endif

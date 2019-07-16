#ifndef YACHTROCK_RUNTIME_H
#define YACHTROCK_RUNTIME_H

#include <yachtrock/results.h>
#include <stdarg.h>
#include <stddef.h>

typedef void (*yr_assertion_failure_callback)(const char *assertion, const char *file, size_t line,
                                              const char *funname, const char *s, va_list ap, void *refcon);
typedef void (*yr_test_skipped_callback)(const char *file, size_t line, const char *funname,
                                         const char *reason, va_list ap, void *refcon);

/**
 * Runtime callbacks.
 *
 * refcon: Arbitrary pointer passed to the callbacks.
 * note_assertion_failed: called when an assertion fails.
 * note_skipped: called when a test is skipped.
 */
struct yr_runtime_callbacks
{
  void *refcon;
  yr_assertion_failure_callback note_assertion_failed;
  yr_test_skipped_callback note_skipped;
};

/**
 * Set the runtime callbacks and return the previously set ones.
 *
 * Callbacks must be set before an assertion is failed or a test is skipped. Yachtrock functions
 * that invoke test cases do this; it's only necessary to call this function if you are implementing
 * your own test running routines.
 */
YACHTROCK_EXTERN struct yr_runtime_callbacks yr_set_runtime_callbacks(struct yr_runtime_callbacks callbacks);

/**
 * Note that an assertion has failed, invoking the runtime callback.
 */
YACHTROCK_EXTERN void yr_fail_assertion(const char *assertion, const char *file, size_t line,
                                        const char *funname, const char *s, ...);
/**
 * Note that a test has been marked as skipped, invoking the runtime callback.
 */
YACHTROCK_EXTERN void yr_skip_test(const char *file, size_t line, const char *funname,
                                   const char *reason, ...);

/**
 * Helper macros.
 */
#define YR_RECORD_SKIP(reason, ...) do {                                \
    yr_skip_test(__FILE__, __LINE__, __FUNCTION__, reason, ##__VA_ARGS__); \
  } while (0)

#define YR_SKIP_AND_RETURN(...) do {            \
    YR_RECORD_SKIP(__VA_ARGS__);                \
    return;                                     \
  } while (0)

#define YR_SKIP_AND_RETURN_UNLESS(test, ...) do {       \
    if ( !(test) ) {                                    \
      YR_SKIP_AND_RETURN(__VA_ARGS__);                  \
    }                                                   \
  } while (0)

#endif

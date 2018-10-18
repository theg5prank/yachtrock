#ifndef YACHTROCK_RUNTIME_H
#define YACHTROCK_RUNTIME_H

#include <yachtrock/results.h>
#include <stdarg.h>
#include <stddef.h>

typedef void (*yr_assertion_failure_callback)(const char *assertion, const char *file, size_t line,
                                              const char *funname, const char *s, va_list ap, void *refcon);
typedef void (*yr_test_skipped_callback)(const char *file, size_t line, const char *funname,
                                         const char *reason, va_list ap, void *refcon);

// callbacks
struct yr_result_callbacks
{
  void *refcon;
  yr_assertion_failure_callback note_assertion_failed;
  yr_test_skipped_callback note_skipped;
};

YACHTROCK_EXTERN struct yr_result_callbacks yr_set_result_callbacks(struct yr_result_callbacks callbacks);


YACHTROCK_EXTERN void yr_fail_assertion(const char *assertion, const char *file, size_t line,
                                        const char *funname, const char *s, ...);
YACHTROCK_EXTERN void yr_skip_test(const char *file, size_t line, const char *funname,
                                   const char *reason, ...);

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

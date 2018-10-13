#ifndef YACHTROCK_RUNTIME_H
#define YACHTROCK_RUNTIME_H

#include <yachtrock/results.h>
#include <stdarg.h>
#include <stddef.h>

typedef void (*yr_assertion_failure_callback)(const char *assertion, const char *file, size_t line,
                                              const char *funname, const char *s, va_list ap, void *refcon);

// callbacks
struct result_callbacks
{
  void *refcon;
  yr_assertion_failure_callback note_assertion_failed;
};

YACHTROCK_EXTERN struct result_callbacks yr_set_result_callbacks(struct result_callbacks callbacks);
YACHTROCK_EXTERN void yr_fail_assertion(const char *assertion, const char *file, size_t line,
                                        const char *funname, const char *s, ...);

#endif

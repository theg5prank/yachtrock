#ifndef YACHTROCK_RUNTIME_H
#define YACHTROCK_RUNTIME_H

#include <yachtrock/results.h>
#include <stdarg.h>
#include <stddef.h>

// callbacks
struct result_callbacks
{
  void *refcon;
  void (*note_assertion_failed)(const char *assertion, const char *file, size_t line,
                                const char *funname, const char *s, va_list ap, void *refcon);
};

YACHTROCK_EXTERN struct result_callbacks yr_set_result_callbacks(struct result_callbacks callbacks);
YACHTROCK_EXTERN void yr_fail_assertion(const char *assertion, const char *file, size_t line,
                                        const char *funname, const char *s, ...);

#endif

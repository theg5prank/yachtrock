#include <yachtrock/runtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "yrutil.h"

static void beartrap_note_assertion_failed(const char *assertion, const char *file,
                                           size_t line, const char *funname,
                                           const char *s, va_list ap, void *refcon)
{
  YR_RUNTIME_ASSERT(false, "assertion failure handler: callbacks not configured");
}

static void beartrap_note_test_skipped(const char *file, size_t line, const char *funname,
                                       const char *reason, va_list ap, void *refcon)
{
  YR_RUNTIME_ASSERT(false, "test skipped handler: callbacks not configured");
}

static struct yr_runtime_callbacks current_callbacks = {
  .refcon = NULL,
  .note_assertion_failed = beartrap_note_assertion_failed,
  .note_skipped = beartrap_note_test_skipped,
};

static void validate_callbacks(struct yr_runtime_callbacks callbacks)
{
  YR_RUNTIME_ASSERT(callbacks.note_assertion_failed != NULL, "assertion failure handler cannot be NULL");
  YR_RUNTIME_ASSERT(callbacks.note_skipped != NULL, "test skipped handler cannot be NULL");
}

struct yr_runtime_callbacks yr_set_runtime_callbacks(struct yr_runtime_callbacks callbacks)
{
  validate_callbacks(callbacks);
  struct yr_runtime_callbacks old = current_callbacks;
  current_callbacks = callbacks;
  return old;
}

void yr_fail_assertion(const char *assertion, const char *file, size_t line,
                       const char *funname, const char *s, ...)
{
  YR_RUNTIME_ASSERT(current_callbacks.note_assertion_failed != NULL,
                    "Can't note assertion failure: handler is NULL!");
  va_list ap;
  va_start(ap, s);
  current_callbacks.note_assertion_failed(assertion, file, line, funname, s, ap, current_callbacks.refcon);
  va_end(ap);
}

void yr_skip_test(const char *file, size_t line, const char *funname,
                  const char *reason, ...)
{
  YR_RUNTIME_ASSERT(current_callbacks.note_skipped != NULL,
                    "Can't note test skipped: handler is NULL!");
  va_list ap;
  va_start(ap, reason);
  current_callbacks.note_skipped(file, line, funname, reason, ap, current_callbacks.refcon);
  va_end(ap);
}

#include <yachtrock/runtime.h>
#include <stdio.h>
#include <stdlib.h>

static void beartrap_note_assertion_failed(const char *assertion, const char *file,
                                           size_t line, const char *funname,
                                           const char *s, va_list ap, void *refcon)
{
  fprintf(stderr, "assertion failure handler: callbacks not configured\n");
  abort();
}

struct result_callbacks current_callbacks = {
  .refcon = NULL,
  .note_assertion_failed = beartrap_note_assertion_failed,
};

static void validate_callbacks(struct result_callbacks callbacks)
{
  if ( callbacks.note_assertion_failed == NULL ) {
    fprintf(stderr, "assertion failure handler cannot be NULL\n");
    abort();
  }
}

struct result_callbacks yr_set_result_callbacks(struct result_callbacks callbacks)
{
  validate_callbacks(callbacks);
  struct result_callbacks old = current_callbacks;
  current_callbacks = callbacks;
  return old;
}

void yr_fail_assertion(const char *assertion, const char *file, size_t line,
                       const char *funname, const char *s, ...)
{
  if ( current_callbacks.note_assertion_failed == NULL ) {
    fprintf(stderr, "Can't note assertion failure: handler is NULL!\n");
    abort();
  }
  va_list ap;
  va_start(ap, s);
  current_callbacks.note_assertion_failed(assertion, file, line, funname, s, ap, current_callbacks.refcon);
  va_end(ap);
}

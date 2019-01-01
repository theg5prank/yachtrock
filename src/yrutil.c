#if __gnu_linux__
#define YR_USE_PROG_INVOK_NAME 1
#define _GNU_SOURCE
#endif


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#include "yrutil.h"

#if !YR_HAVE_ERR_H || !YR_HAVE_ERRC

/* This implementation may truncate the error message.
 * That is because the strerror_r interface is an enormous turd.
 */
#define ERRBUF_SIZE 128
static void emit_diagnostic(int code, const char *fmt, va_list ap)
{
  char buf[128];
  strerror_r(errno, buf, sizeof(buf));
#if YR_USE_PROG_INVOK_NAME
  fprintf(stderr, "%s: ", program_invocation_name);
#endif
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, ": ");
  fprintf(stderr, "%s\n", buf);
}

#endif // !YR_HAVE_ERR_H || !YR_HAVE_ERRC

#if !YR_HAVE_ERR_H

void yr_err(int eval, const char *fmt, ...)
{
  int code = errno;
  va_list ap;
  va_start(ap, fmt);
  emit_diagnostic(code, fmt, ap);
  va_end(ap);
  exit(eval);
}

void yr_errx(int eval, const char *fmt, ...)
{
  va_list ap;
#if YR_USE_PROG_INVOK_NAME
  fprintf(stderr, "%s: ", program_invocation_name);
#endif
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
  exit(eval);
}

void yr_warn(const char *fmt, ...)
{
  int code = errno;
  va_list ap;
  va_start(ap, fmt);
  emit_diagnostic(code, fmt, ap);
  va_end(ap);
}

void yr_warnx(const char *fmt, ...)
{
  va_list ap;
#if YR_USE_PROG_INVOK_NAME
  fprintf(stderr, "%s: ", program_invocation_name);
#endif
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
}

#endif // !YR_HAVE_ERR_H

#if !YR_HAVE_ERRC

void yr_errc(int eval, int code, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  emit_diagnostic(code, fmt, ap);
  va_end(ap);
  exit(eval);
}

void yr_warnc(int code, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  emit_diagnostic(code, fmt, ap);
  va_end(ap);
}

#endif // YR_HAVE_ERR_H && !YR_HAVE_ERRC

char *yr_strdup(const char *in)
{
  YR_RUNTIME_ASSERT(in, "%s called with NULL", __FUNCTION__);
  char *result = malloc(strlen(in) + 1);
  strcpy(result, in);
  return result;
}

void __yr_runtime_assert_fail__(char *fmt, ...)
{
  fprintf(stderr, "yachtrock runtime assertion failed: ");
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fprintf(stderr, "\nbreak on %s to debug\n", __FUNCTION__);
  abort();
}

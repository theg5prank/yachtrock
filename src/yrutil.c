#if __gnu_linux__
#define YR_USE_PROG_INVOK_NAME 1
#define _GNU_SOURCE
#endif


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <yachtrock/yachtrock.h>

#include "yrutil.h"

#if YR_USE_STDTHREADS
#include <threads.h>
typedef once_flag yr_once_t;
#define YR_ONCE_INIT ONCE_FLAG_INIT
#define yr_once call_once
#define YR_ONCE_AVAILABLE 1
#elif YACHTROCK_POSIXY
#include <pthread.h>
typedef pthread_once_t yr_once_t;
#define YR_ONCE_INIT PTHREAD_ONCE_INIT
#define yr_once pthread_once
#define YR_ONCE_AVAILABLE 1
#else
#define YR_ONCE_AVAILABLE 0
#endif

#if !YR_HAVE_ERR_H || !YR_HAVE_ERRC

/* This implementation may truncate the error message.
 * That is because the strerror_r interface is an enormous turd.
 */
#define ERRBUF_SIZE 128
static void emit_diagnostic(int code, const char *fmt, va_list ap)
{
  char buf[ERRBUF_SIZE];
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
  char *result = yr_malloc(strlen(in) + 1);
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

static void yr_allocation_failure(void)
{
  sleep(1);
}

void *yr_malloc(size_t size)
{
  void *result = NULL;
  do {
    result = malloc(size);
  } while ( result == NULL && (yr_allocation_failure(), true) );
  return result;
}

void *yr_calloc(size_t size, size_t nobj)
{
  void *result = NULL;
  do {
    result = calloc(size, nobj);
  } while ( result == NULL && (yr_allocation_failure(), true) );
  return result;
}

void *yr_realloc(void *ptr, size_t size)
{
  void *result = NULL;
  do {
    result = realloc(ptr, size);
  } while ( (result == NULL && size != 0) && (yr_allocation_failure(), true) );
  return result;
}

#if YR_ONCE_AVAILABLE
static bool use_terminal_color;
static void init_use_terminal_color(void)
{
  char *term = getenv("TERM");
  use_terminal_color = isatty(2) && term && term[0] && strcmp(term, "dumb");
}
#endif

bool yr_use_terminal_color(void)
{
#if YR_ONCE_AVAILABLE
  static yr_once_t once = YR_ONCE_INIT;
  yr_once(&once, init_use_terminal_color);
  return use_terminal_color;
#else
  return 0;
#endif
}

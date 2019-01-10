#ifndef YACHTROCK_YRUTIL_H
#define YACHTROCK_YRUTIL_H

#include <stdnoreturn.h>
#include <errno.h>

#ifdef __has_include
#  if __has_include(<err.h>)
#    define YR_HAVE_ERR_H 1
#  else
#    define YR_HAVE_ERR_H 0
#  endif
#elif defined(BSD) || (__APPLE__ && __MACH__)
#  define YR_HAVE_ERR_H 1
#else
#  define YR_HAVE_ERR_H 0
#endif

#if YR_HAVE_ERR_H
#  if defined(BSD) || (__APPLE__ && __MACH__)
#    define YR_HAVE_ERRC 1
#  else
#    define YR_HAVE_ERRC 0
#  endif
#endif

#if !YR_HAVE_ERR_H

extern void yr_err(int eval, const char *fmt, ...);
extern void yr_errx(int eval, const char *fmt, ...);

extern void yr_warn(const char *fmt, ...);
extern void yr_warnx(const char *fmt, ...);

#else

#include <err.h>

#define yr_err err
#define yr_errx errx
#define yr_warn warn
#define yr_warnx warnx

#endif // !YR_HAVE_ERR_H

#if !YR_HAVE_ERRC

extern void yr_errc(int eval, int code, const char *fmt, ...);
extern void yr_warnc(int code, const char *fmt, ...);

#else

#define yr_errc errc
#define yr_warnc warnc

#endif // !YR_HAVE_ERRC

#define EINTR_RETRY(expr) do {                  \
    if ( (expr) >= 0 || errno != EINTR ) {      \
      break;                                    \
    }                                           \
  } while (1)

extern char *yr_strdup(const char *in);
extern noreturn void __yr_runtime_assert_fail__(char *fmt, ...);

#define YR_RUNTIME_ASSERT(test, ...) do {       \
    if ( !(test) ) {                            \
      __yr_runtime_assert_fail__(__VA_ARGS__);  \
    }                                           \
  } while ( 0 )

extern void *yr_malloc(size_t size);
extern void *yr_calloc(size_t size, size_t nobj);
extern void *yr_realloc(void *ptr, size_t size);

#if (__STDC_VERSION__ >= 201112L) && !__STDC_NO_ATOMICS__
#define YR_USE_STDATOMIC 1
#endif

#if (__STDC_VERSION__ >= 201112L) && !__STDC_NO_THREADS__
#define YR_USE_STDTHREADS 1
#endif

#endif

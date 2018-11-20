#ifndef YACHTROCK_YRUTIL_H
#define YACHTROCK_YRUTIL_H

extern char *yr_strdup(const char *in);

extern void __yr_runtime_assert_fail__(char *fmt, ...);

#define YR_RUNTIME_ASSERT(test, ...) do {       \
    if ( !(test) ) {                            \
      __yr_runtime_assert_fail__(__VA_ARGS__);  \
    }                                           \
  } while ( 0 )

#endif

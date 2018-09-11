#ifndef YACHTROCK_ASSERT_H
#define YACHTROCK_ASSERT_H

#include <yachtrock/runtime.h>

#define YR_ASSERT(test, s, ...) do {                                    \
    if ( !(test) ) {                                                    \
      yr_fail_assertion(#test, __FILE__, __LINE__, __FUNCTION__, (s), ##__VA_ARGS__); \
    }                                                                   \
  } while (0)

#endif

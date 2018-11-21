#ifndef YACHTROCK_BASE_H
#define YACHTROCK_BASE_H

#define YACHTROCK_EXTERN extern

#define YACHTROCK_UNIXY (__unix__ || __linux__ || (__APPLE__ && __MACH__) || BSD)
#if YACHTROCK_UNIXY
#include <unistd.h>
#endif

#if _POSIX_VERSION
#define YACHTROCK_POSIXY 1
#else
#define YACHTROCK_POSIXY 0
#endif

#define YACHTROCK_DLOPEN YACHTROCK_POSIXY && (_POSIX_VERSION > 200100)

#define YR_STR(X) #X
#define YR_XSTR(X) YR_STR(X)

#endif

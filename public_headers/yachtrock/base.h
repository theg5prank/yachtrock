#ifndef YACHTROCK_BASE_H
#define YACHTROCK_BASE_H

#include <yachtrock/config.h>

#ifdef __cplusplus
#define YACHTROCK_EXTERN extern "C"
#define YACHTROCK_NORETURN
#define YACHTROCK_RESTRICT
#else
#define YACHTROCK_EXTERN extern
#include <stdnoreturn.h>
#define YACHTROCK_NORETURN noreturn
#define YACHTROCK_RESTRICT restrict
#endif

#define YR_STR(X) #X
#define YR_XSTR(X) YR_STR(X)

#endif

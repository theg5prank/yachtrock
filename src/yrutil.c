#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "yrutil.h"

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

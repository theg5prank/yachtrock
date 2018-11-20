#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "yrutil.h"

char *yr_strdup(const char *in)
{
  if ( !in ) {
    fprintf(stderr, "%s called with NULL\n", __FUNCTION__);
    abort();
  }
  char *result = malloc(strlen(in) + 1);
  strcpy(result, in);
  return result;
}

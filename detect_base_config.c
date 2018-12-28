#include <stdio.h>
#include <stdlib.h>

#if (__unix__ || __linux__ || (__APPLE__ && __MACH__) || BSD)
#define YACHTROCK_UNIXY 1
#else
#define YACHTROCK_UNIXY 0
#endif

#if YACHTROCK_UNIXY
#include <unistd.h>
#endif

#if _POSIX_VERSION
#define YACHTROCK_POSIXY 1
#else
#define YACHTROCK_POSIXY 0
#endif

#if (YACHTROCK_POSIXY && (_POSIX_VERSION > 200100))
#define YACHTROCK_MULTIPROCESS 1
#else
#define YACHTROCK_MULTIPROCESS 0
#endif

#if YACHTROCK_POSIXY && (_POSIX_VERSION > 200100)
#define YACHTROCK_DLOPEN 1
#else
#define YACHTROCK_DLOPEN 0
#endif

#define STR(X) #X
#define XSTR(X) STR(X)

int main(void)
{
  struct binding { char *name; unsigned long value; };
#define BINDING(n) { .name = #n, .value = (getenv(#n) ? strtoul(getenv(#n), NULL, 0) : n) }
  struct binding bindings[] =  {
    BINDING(YACHTROCK_UNIXY),
    BINDING(YACHTROCK_POSIXY),
    BINDING(YACHTROCK_DLOPEN),
    BINDING(YACHTROCK_MULTIPROCESS)
  };
  for ( int i = 0; i < sizeof(bindings) / sizeof(bindings[0]); i++ ) {
    fprintf(stdout, "%s=%lu\n", bindings[i].name, bindings[i].value);
  }
  return EXIT_SUCCESS;
}

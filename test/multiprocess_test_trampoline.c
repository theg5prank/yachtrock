#include <stdlib.h>

#include "multiprocess_tests.h"

int main(int argc, char **argv)
{
  if ( !yr_process_is_inferior() ) {
    fprintf(stderr, "%s: not run as inferior?\n", argv[0]);
    abort();
  }

  char *collname = getenv(INFERIOR_COLLECTION_NAME_VAR);

  yr_test_suite_collection_t collection = collname ?
    yr_multiprocess_test_inferior_create_collection(collname) : NULL;

  if ( !collection ) {
    fprintf(stderr, "%s: couldn't get collection named %s\n", argv[0], collname ? collname : "(null)");
    abort();
  }

  yr_inferior_checkin(collection, YR_BASIC_STDERR_RUNTIME_CALLBACKS);
  /* unreachable. */
  return EXIT_FAILURE;
}

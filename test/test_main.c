#include "yrtests.h"

extern char **environ;

int main(int argc, char **argv)
{
  yr_test_suite_collection_t collection = yachtrock_selftests_collection();
  yr_result_store_t store = yr_result_store_create_with_hooks("yachtrock self-tests",
                                                              YR_BASIC_STDERR_RESULT_HOOKS);
#if YACHTROCK_MULTIPROCESS
  yr_run_suite_collection_under_store_multiprocess(argv[0], argv, environ, collection,
                                                   store, YR_BASIC_STDERR_RUNTIME_CALLBACKS);
#else
  yr_run_suite_collection_under_store(collection, store, YR_BASIC_STDERR_RUNTIME_CALLBACKS);
#endif
  yr_result_store_close(store);
  free(collection);

  char *desc = yr_result_store_copy_description(store);
  fprintf(stdout, "%s\n", desc);
  free(desc);
  yr_result_t result = yr_result_store_get_result(store);
  yr_result_store_destroy(store);

  return result == YR_RESULT_PASSED ? EXIT_SUCCESS : EXIT_FAILURE;
}

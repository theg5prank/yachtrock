#include <yachtrock/yachtrock.h>
#include <unistd.h>
#include <stdio.h>

#if YACHTROCK_DLOPEN

#include <stdlib.h>
#include <assert.h>
#include <dlfcn.h>

extern char **environ;

const char *progn;

static yr_test_suite_collection_t create_collection_from_path(const char *path)
{
  /* Note that the handle is intentionally leaked. */
  void *handle = dlopen(path, RTLD_NOW);
  if ( handle == NULL ) {
    char *error = dlerror();
    fprintf(stderr, "%s: Couldn't load image %s: %s\n", progn, path, error);
    return NULL;
  }
  char *errmsg = NULL;
  yr_test_suite_collection_t collection = yr_test_suite_collection_load_from_handle(handle, &errmsg);
  if ( collection == NULL ) {
    fprintf(stderr, "%s: Couldn't load collection from image at %s: %s\n", progn, path, errmsg);
  }
  return collection;
}

int main(int argc, char **argv)
{
  if ( argc <= 1 ) {
    fprintf(stderr, "%s: no images provided to load tests from\n", argv[0]);
    return EXIT_FAILURE;
  }
  progn = argv[0];
  yr_test_suite_collection_t collections[argc - 1];
  int num_collections_loaded = 0;
  for ( int i = 1; i < argc; i++ ) {
    yr_test_suite_collection_t collection = create_collection_from_path(argv[i]);
    if ( collection != NULL ) {
      collections[num_collections_loaded++] = collection;
    }
  }

  yr_test_suite_collection_t final_collection =
    yr_test_suite_collection_create_from_collection_array(num_collections_loaded, collections);
  for ( int i = 0; i < num_collections_loaded; i++ ) {
    assert(collections[i]);
    free(collections[i]);
  }

  struct yr_result_hooks hooks = (yr_process_is_inferior() ?
                                  (struct yr_result_hooks){0} : YR_BASIC_STDERR_RESULT_HOOKS);
  yr_result_store_t store = yr_result_store_create_with_hooks("tests run from images",
                                                              hooks);

  yr_run_suite_collection_under_store_multiprocess(argv[0], argv, environ,
                                                   final_collection, store,
                                                   YR_BASIC_STDERR_RUNTIME_CALLBACKS);

  if ( !yr_process_is_inferior() ) {
    char *desc = yr_result_store_copy_description(store);
    fprintf(stdout, "%s\n", desc);
    free(desc);
  }

  yr_result_t result = yr_result_store_get_result(store);

  return result == YR_RESULT_PASSED ? EXIT_SUCCESS : EXIT_FAILURE;
}

#else

#include <stdio.h>

int main(int argc, char **argv)
{
  fprintf(stderr, "%s was not built with dynamic linking support.\n", argv[0]);
  return EXIT_FAILURE;
}

#endif

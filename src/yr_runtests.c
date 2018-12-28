#include <yachtrock/yachtrock.h>
#include <unistd.h>
#include <stdio.h>
#include <sysexits.h>

#include <stdlib.h>
#include <assert.h>
#include <dlfcn.h>

#include "yrutil.h"

extern char **environ;

const char *progn;

static void print_usage(void)
{
  fprintf(stderr, "Usage: %s [-m | -u ] [-n NAME] DYLIB...\n", progn);
}

static char **copy_argv(char **input)
{
  size_t entries = 0; /* includes NULL terminator */
  while( input[entries++] );
  char **retval = malloc(sizeof(char *) * entries);
  for ( size_t i = 0; i < entries; i++ ) {
    retval[i] = input[i] ? yr_strdup(input[i]) : NULL;
  }
  return retval;
}

static void free_copied_argv(char **input)
{
  for ( char **iter = input; *iter; iter++ ) {
    free(*iter);
  }
  free(input);
}

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
  yr_test_suite_collection_t collection = yr_test_suite_collection_create_from_handle(handle, &errmsg);
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

  // glibc says its getopt munges argv. *shrug*
  char **argv_copy = copy_argv(argv);

  char *name = NULL;
  int ch;
  bool multiprocess = YACHTROCK_MULTIPROCESS;
  while ( (ch = getopt(argc, argv_copy, "umn:")) != -1 ) {
    switch ( ch ) {
    case 'u':
      multiprocess = false;
      break;
    case 'm':
      multiprocess = true;
      break;
    case 'n':
      name = optarg;
      break;
    case '?':
    default:
      print_usage();
      return EX_USAGE;
    }
  }

  name = name ? name : "tests run from images";

  yr_test_suite_collection_t collections[argc - 1];
  int num_collections_loaded = 0;
  for ( int i = optind; i < argc; i++ ) {
    yr_test_suite_collection_t collection = create_collection_from_path(argv_copy[i]);
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

  bool inferior =
#if YACHTROCK_MULTIPROCESS
    yr_process_is_inferior();
#else
    false;
#endif

  struct yr_result_hooks hooks = (inferior ?
                                  (struct yr_result_hooks){0} : YR_BASIC_STDERR_RESULT_HOOKS);
  yr_result_store_t store = yr_result_store_create_with_hooks(name, hooks);

  if ( multiprocess ) {
#if YACHTROCK_MULTIPROCESS
    yr_run_suite_collection_under_store_multiprocess(argv[0], argv, environ,
                                                     final_collection, store,
                                                     YR_BASIC_STDERR_RUNTIME_CALLBACKS);
#else
    fprintf(stderr, "%s: Not built with multiprocess support.\n", progn);
    return EX_UNAVAILABLE;
#endif
  } else {
    yr_run_suite_collection_under_store(final_collection, store, YR_BASIC_STDERR_RUNTIME_CALLBACKS);
  }

  yr_result_store_close(store);

  if ( !inferior ) {
    char *desc = yr_result_store_copy_description(store);
    fprintf(stdout, "%s\n", desc);
    free(desc);
  }

  yr_result_t result = yr_result_store_get_result(store);

  yr_result_store_destroy(store);
  free(final_collection);

  free_copied_argv(argv_copy);

  return result == YR_RESULT_PASSED ? EXIT_SUCCESS : EXIT_FAILURE;
}

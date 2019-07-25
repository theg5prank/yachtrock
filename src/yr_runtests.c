#include <yachtrock/yachtrock.h>
#include <unistd.h>
#include <stdio.h>
#include <sysexits.h>

#include <stdlib.h>
#include <assert.h>
#include <dlfcn.h>

#include "yrutil.h"
#include "version_internal.h"

extern const char *yr_runtests_what;
const char *yr_runtests_what =
  "@(#) PROGRAM:yr_runtests  PROJECT:libyachtrock " YR_VERSION_LITERAL;

extern char **environ;

static const char *progn;

static void print_usage(void)
{
  fprintf(stderr, "Usage: %s [-m | -u ] [-n NAME] [-g GLOB]... DYLIB...\n", progn);
}

static char **copy_argv(char **input)
{
  size_t entries = 0; /* includes NULL terminator */
  while( input[entries++] );
  char **retval = yr_malloc(sizeof(char *) * entries);
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

static void add_selector(yr_selector_t selector, yr_selector_t **selectors, size_t *num_selectors,
                         size_t *selectors_capacity)
{
  assert(*num_selectors <= *selectors_capacity);
  if ( *num_selectors == *selectors_capacity ) {
    *selectors_capacity = *selectors_capacity > 0 ? *selectors_capacity * 2 : 4;
    *selectors = yr_realloc(*selectors, sizeof(yr_selector_t) * *selectors_capacity);
  }
  (*selectors)[(*num_selectors)++] = selector;
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
  progn = argv[0];

  if ( argc <= 1 ) {
    fprintf(stderr, "%s: no images provided to load tests from\n", argv[0]);
    print_usage();
    return EXIT_FAILURE;
  }

  // glibc says its getopt munges argv. *shrug*
  char **argv_copy = copy_argv(argv);

  char *name = NULL;
  int ch;
  bool multiprocess = YACHTROCK_MULTIPROCESS;

  yr_selector_t *selectors = NULL;
  size_t num_selectors = 0;
  size_t selectors_capacity = 0;
  yr_selector_set_t selector_set = NULL;

  while ( (ch = getopt(argc, argv_copy, "umn:g:")) != -1 ) {
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
    case 'g':
      {
        yr_selector_t selector = yr_selector_create_from_glob(optarg);
        add_selector(selector, &selectors, &num_selectors, &selectors_capacity);
        break;
      }
    case '?':
    default:
      print_usage();
      return EX_USAGE;
    }
  }

  if ( num_selectors > 0 ) {
    selector_set = yr_selector_set_create(num_selectors, selectors);
    for ( size_t i = 0; i < num_selectors; i++ ) {
      yr_selector_destroy(selectors[i]);
    }
    free(selectors);
    selectors = NULL;
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

  if ( num_collections_loaded == 0 ) {
    fprintf(stderr, "%s: all collections failed to load.\n", progn);
    return EX_DATAERR;
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

  if ( selector_set ) {
    yr_test_suite_collection_t temp = final_collection;
    final_collection = yr_test_suite_collection_create_filtered(temp, selector_set);
    free(temp);
    yr_selector_set_destroy(selector_set);
    selector_set = NULL;

    if ( final_collection == NULL ) {
      fprintf(stderr, "%s: all tests were filtered out.\n", progn);
      return EX_DATAERR;
    }
  }

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
  /* Note that the result store may be initialized with a name drawn from argv_copy */
  free_copied_argv(argv_copy);

  return result == YR_RESULT_PASSED ? EXIT_SUCCESS : EXIT_FAILURE;
}

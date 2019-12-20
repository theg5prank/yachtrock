#include <yachtrock/yachtrock.h>
#include <unistd.h>
#include <stdio.h>
#include <sysexits.h>

#include <stdlib.h>
#include <assert.h>
#include <dlfcn.h>

#include "yrutil.h"
#include "version_internal.h"

extern const char * const yr_runtests_what;
const char * const yr_runtests_what =
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

struct parsed_args {
  yr_test_suite_collection_t suite_collection;
  char *name;
  bool multiprocess;
};

static yr_selector_set_t
create_selector_set_consuming_selector_list(yr_selector_t **io_selectors,
                                            size_t num_selectors)
{
  yr_selector_set_t result = NULL;
  if ( num_selectors > 0 ) {
    result = yr_selector_set_create(num_selectors, *io_selectors);
    for ( size_t i = 0; i < num_selectors; i++ ) {
      yr_selector_destroy((*io_selectors)[i]);
    }
    free(*io_selectors);
    *io_selectors = NULL;
  }

  return result;
}

static void
create_filtered_suite_collection_consuming_collection(yr_test_suite_collection_t *io_collection,
                                                      yr_selector_set_t set)
{
  if ( set ) {
    yr_test_suite_collection_t temp = *io_collection;
    *io_collection = yr_test_suite_collection_create_filtered(temp, set);
    free(temp);
  }
}

static void dispose_parsed_args(struct parsed_args *inout_parsed_args)
{
  free(inout_parsed_args->suite_collection);
  inout_parsed_args->suite_collection = NULL;

  free(inout_parsed_args->name);
  inout_parsed_args->name = NULL;
}

static size_t create_collections_array(size_t count, char **paths,
                                       yr_test_suite_collection_t **out_collections)
{
  size_t num_collections_loaded = 0;
  *out_collections = malloc(sizeof(**out_collections) * count);
  for ( size_t i = 0; i < count; i++ ) {
    yr_test_suite_collection_t collection = create_collection_from_path(paths[i]);
    if ( collection != NULL ) {
      (*out_collections)[num_collections_loaded++] = collection;
    }
  }
  return num_collections_loaded;
}

static void destroy_collections_array(yr_test_suite_collection_t *collections, size_t count)
{
  for ( size_t i = 0; i < count; i++ ) {
    assert(collections[i]);
    free(collections[i]);
  }

  free(collections);
}

static int parse_args(int argc, char **argv, struct parsed_args *out_parsed_args)
{
  int result = EXIT_SUCCESS;

  char *name = NULL;
  int ch;

  yr_selector_t *selectors = NULL;
  size_t num_selectors = 0;
  size_t selectors_capacity = 0;
  yr_selector_set_t selector_set = NULL;
  bool multiprocess = YACHTROCK_MULTIPROCESS;

  char **argv_copy = copy_argv(argv);

  yr_test_suite_collection_t *collections = NULL;
  size_t num_collections_loaded = 0;
  yr_test_suite_collection_t final_collection = NULL;

  while ( (ch = getopt(argc, argv_copy, "umn:g:")) != -1 ) {
    switch ( ch ) {
    case 'u':
      multiprocess = false;
      break;
    case 'm':
      multiprocess = true;
      break;
    case 'n':
      name = yr_strdup(optarg);
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
      result = EX_USAGE;
      goto out;
    }
  }

  if ( argc <= optind ) {
    fprintf(stderr, "%s: no images provided to load tests from\n", argv[0]);
    print_usage();
    result = EXIT_FAILURE;
    goto out;
  }

  selector_set = create_selector_set_consuming_selector_list(&selectors, num_selectors);

  name = name ? name : yr_strdup("tests run from images");

  num_collections_loaded = create_collections_array(argc - optind, argv_copy + optind,
                                                    &collections);

  if ( num_collections_loaded == 0 ) {
    fprintf(stderr, "%s: all collections failed to load.\n", progn);
    result = EX_DATAERR;
    goto out;
  }

  final_collection =
    yr_test_suite_collection_create_from_collection_array(num_collections_loaded, collections);

  create_filtered_suite_collection_consuming_collection(&final_collection, selector_set);

out:
  if ( selector_set ) {
    yr_selector_set_destroy(selector_set);
  }

  destroy_collections_array(collections, num_collections_loaded);

  free_copied_argv(argv_copy);

  if ( result == EXIT_SUCCESS ) {
    out_parsed_args->suite_collection = final_collection;
    out_parsed_args->name = name;
    out_parsed_args->multiprocess = multiprocess;
  } else {
    free(final_collection);
    free(name);
  }
  return result;
}

int main(int argc, char **argv)
{
  progn = argv[0];

  struct parsed_args parsed_args;
  int parse_result = parse_args(argc, argv, &parsed_args);
  if ( parse_result != EXIT_SUCCESS ) {
    return parse_result;
  }

  bool inferior =
#if YACHTROCK_MULTIPROCESS
    yr_process_is_inferior()
#else
    false
#endif
    ;

  if ( parsed_args.suite_collection == NULL ) {
    fprintf(stderr, "%s: all tests were filtered out.\n", progn);
    return EX_DATAERR;
  }

  struct yr_result_hooks hooks = (inferior ?
                                  (struct yr_result_hooks){0} : YR_BASIC_STDERR_RESULT_HOOKS);
  yr_result_store_t store = yr_result_store_create_with_hooks(parsed_args.name, hooks);

  if ( parsed_args.multiprocess ) {
#if YACHTROCK_MULTIPROCESS
    yr_run_suite_collection_under_store_multiprocess(argv[0], argv, environ,
                                                     parsed_args.suite_collection, store,
                                                     YR_BASIC_STDERR_RUNTIME_CALLBACKS);
#else
    fprintf(stderr, "%s: Not built with multiprocess support.\n", progn);
    return EX_UNAVAILABLE;
#endif
  } else {
    yr_run_suite_collection_under_store(parsed_args.suite_collection, store,
                                        YR_BASIC_STDERR_RUNTIME_CALLBACKS);
  }

  yr_result_store_close(store);

  if ( !inferior ) {
    char *desc = yr_result_store_copy_description_ansi(store, yr_use_terminal_color());
    fprintf(stdout, "%s\n", desc);
    free(desc);
  }

  yr_result_t result = yr_result_store_get_result(store);

  yr_result_store_destroy(store);

  dispose_parsed_args(&parsed_args);

  return result == YR_RESULT_PASSED ? EXIT_SUCCESS : EXIT_FAILURE;
}

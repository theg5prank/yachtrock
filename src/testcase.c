#include <yachtrock/testcase.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdalign.h>

#if YACHTROCK_DLOPEN
#include <dlfcn.h>
#endif

#include "yrutil.h"

const struct yr_suite_lifecycle_callbacks YR_NO_CALLBACKS = {0};

static bool isdelim(int c)
{
  return isspace(c) || c == ',';
}

static const char *next_name(const char *input, const char **after_end)
{
  while ( *input && isdelim(*input) ) {
    input++;
  }
  if ( *input == '\0' ) {
    return NULL;
  }
  const char *start = input;
  while ( *input && !isdelim(*input) ) {
    input++;
  }
  *after_end = input;
  return start;
}

static size_t ginormous_namestring_size(const char *cs_names)
{
  size_t result = 0;
  const char *after_end = cs_names;
  do {
    cs_names = next_name(cs_names, &after_end);
    result += after_end - cs_names + 1;
    cs_names = after_end;
  } while ( *cs_names != '\0' );
  return result;
}

static size_t num_names(const char *cs_names)
{
  size_t num = 1;
  for ( ; *cs_names; cs_names++ ) {
    if ( *cs_names == ',' ) {
      num++;
    }
  }
  return num;
}

yr_test_suite_t
_yr_create_suite_from_functions(const char *name,
                                void *suite_refcon,
                                struct yr_suite_lifecycle_callbacks callbacks,
                                const char *cs_names,
                                yr_test_case_function first, ...)
{
  size_t n_names = num_names(cs_names);
  size_t names_size = ginormous_namestring_size(cs_names);
  yr_test_suite_t suite = calloc(sizeof(yr_test_suite_s) + sizeof(yr_test_case_s) * n_names + names_size, 1);
  suite->name = name;
  suite->refcon = suite_refcon;
  suite->lifecycle = callbacks;
  suite->num_cases = n_names;

  char *next_name_destination_start = (char *)(&(suite->cases[n_names]));
  const char *last_name_source_after_end = cs_names;

  va_list ap;

  for ( size_t i = 0; i < n_names; i++ ) {
    const char *next_name_after_end = NULL;
    const char *next_name_source = next_name(last_name_source_after_end, &next_name_after_end);
    assert(next_name_source != NULL);
    size_t len = next_name_after_end - next_name_source;
    memcpy(next_name_destination_start, next_name_source, len);
    next_name_destination_start[len] = '\0';
    suite->cases[i].name = next_name_destination_start;
    yr_test_case_function testcase = NULL;
    if ( i == 0 ) {
      testcase = first;
      if ( n_names > 1 ) {
        va_start(ap, first);
      }
    } else {
      testcase = va_arg(ap, yr_test_case_function);
    }
    suite->cases[i].testcase = testcase;
    suite->cases[i].suite = suite;

    next_name_destination_start = next_name_destination_start + len + 1;
    last_name_source_after_end = next_name_after_end;
  }

  if ( n_names > 1 ) {
    va_end(ap);
  }

  return suite;
}

yr_test_suite_t
yr_create_blank_suite(size_t num_cases)
{
  yr_test_suite_t suite = calloc(sizeof(yr_test_suite_s) + sizeof(yr_test_case_s) * num_cases, 1);
  suite->num_cases = num_cases;
  for ( size_t i = 0; i < num_cases; i++ ) {
    suite->cases[i].suite = suite;
  }
  return suite;
}

static const size_t SUITE_ALIGNMENT = alignof(struct yr_test_suite);
static size_t test_suite_string_storage_size(yr_test_suite_t suite)
{
  size_t necessary = strlen(suite->name) + 1;
  for ( size_t i = 0; i < suite->num_cases; i++ ) {
    necessary += strlen(suite->cases[i].name) + 1;
  }
  return necessary;
}
static size_t test_suite_total_size(yr_test_suite_t suite)
{
  /* include padding at the end of the string storage to allow for arrays of suites
   * that have their own string storage. For now. Until I want to reclaim those 7*n bytes
   */
  size_t strictly_necessary = (sizeof(struct yr_test_suite) +
                               sizeof(struct yr_test_case) * suite->num_cases +
                               test_suite_string_storage_size(suite));
  size_t remainder = strictly_necessary % SUITE_ALIGNMENT;
  size_t padding = remainder == 0 ? 0 : (strictly_necessary - remainder) + SUITE_ALIGNMENT;
  return strictly_necessary + padding;
}
static char *test_suite_string_storage_location(yr_test_suite_t suite)
{
  return (char *)&(suite->cases[suite->num_cases]);
}
static void copy_test_suite(yr_test_suite_t dst, yr_test_suite_t src)
{
  /* copy everything including name POINTERS first. That lets us calculate where the string part is
   * so we can do the real copying of the names.
   */
  dst->refcon = src->refcon;
  dst->lifecycle = src->lifecycle;
  dst->num_cases = src->num_cases;
  for ( size_t i = 0; i < src->num_cases; i++ ) {
    dst->cases[i].testcase = src->cases[i].testcase;
    dst->cases[i].suite = dst;
  }

  char *test_suite_string_storage_insertion_point = test_suite_string_storage_location(dst);
  strcpy(test_suite_string_storage_insertion_point, src->name);
  dst->name = test_suite_string_storage_insertion_point;
  test_suite_string_storage_insertion_point += strlen(test_suite_string_storage_insertion_point) + 1;

  for ( size_t i = 0; i < src->num_cases; i++ ) {
    strcpy(test_suite_string_storage_insertion_point, src->cases[i].name);
    dst->cases[i].name = test_suite_string_storage_insertion_point;
    test_suite_string_storage_insertion_point += strlen(test_suite_string_storage_insertion_point) + 1;
  }

  assert(test_suite_string_storage_insertion_point - (char *)dst <= test_suite_total_size(src));
  assert(test_suite_total_size(src) == test_suite_total_size(dst));
}

yr_test_suite_collection_t
yr_test_suite_collection_create_from_suites(size_t num_suites, yr_test_suite_t *suites)
{
  size_t suite_memory_needed = 0;
  for ( size_t i = 0; i < num_suites; i++ ) {
    suite_memory_needed += test_suite_total_size(suites[i]);
  }
  size_t pointer_memory_needed = sizeof(yr_test_suite_t) * num_suites;
  size_t suite_alignment_padding = SUITE_ALIGNMENT - ( pointer_memory_needed % SUITE_ALIGNMENT );
  pointer_memory_needed += suite_alignment_padding;
  yr_test_suite_collection_t collection = malloc(sizeof(struct yr_test_suite_collection) +
                                                 pointer_memory_needed +
                                                 suite_memory_needed);
  collection->num_suites = num_suites;

  void *next_suite_placement = (char *)(&collection->suites[num_suites]) + suite_alignment_padding;
  for ( size_t i = 0; i < num_suites; i++ ) {
    yr_test_suite_t suite = next_suite_placement;
    copy_test_suite(suite, suites[i]);
    collection->suites[i] = suite;

    next_suite_placement = (char *)next_suite_placement + test_suite_total_size(suites[i]);
  }
  return collection;
}

yr_test_suite_collection_t
yr_test_suite_collection_create_from_collections(size_t num_collections,
                                                 yr_test_suite_collection_t collection1,
                                                 ...)
{
  // collect all collections
  yr_test_suite_collection_t collections[num_collections];
  collections[0] = collection1;
  va_list ap;
  va_start(ap, collection1);
  for ( size_t i = 1; i < num_collections; i++ ) {
    yr_test_suite_collection_t collection = va_arg(ap, yr_test_suite_collection_t);
    collections[i] = collection;
  }
  va_end(ap);

  // collect all suites
  size_t num_suites = 0;
  for ( size_t i = 0; i < num_collections; i++ ) {
    num_suites += collections[i]->num_suites;
  }

  yr_test_suite_t suites[num_suites];
  size_t suite_index = 0;
  for ( size_t collection_index = 0; collection_index < num_collections; collection_index++ ) {
    for ( size_t collection_suite_index = 0;
          collection_suite_index < collections[collection_index]->num_suites;
          collection_suite_index++ ) {
      suites[suite_index++] = collections[collection_index]->suites[collection_suite_index];
    }
  }

  // and now we have what we need
  return yr_test_suite_collection_create_from_suites(num_suites, suites);
}

#if YACHTROCK_DLOPEN

static const char discoverer_sym[] = YR_XSTR(YACHTROCK_MODULE_DISCOVER_NAME);

yr_test_suite_collection_t
yr_test_suite_collection_create_from_dylib_path(const char *path, char **errmsg)
{
  yr_test_suite_collection_t result = NULL;
  void *handle = dlopen(path, RTLD_LAZY);
  if ( !handle ) {
    if ( errmsg ) {
      *errmsg = yr_strdup(dlerror());
    }
    goto out;
  }

  result = yr_test_suite_collection_create_from_handle(handle, errmsg);

 out:
  if ( !result && handle ) {
    dlclose(handle);
  }
  return result;
}

yr_test_suite_collection_t
yr_test_suite_collection_create_from_handle(void *handle, char **errmsg)
{
  yr_test_suite_collection_t result = NULL;
  yr_module_discoverer_t discoverer = dlsym(handle, discoverer_sym);
  if ( !discoverer ) {
    if ( errmsg ) {
      *errmsg = yr_strdup(dlerror());
    }
    goto out;
  }

  char *discover_err = NULL;
  result = discoverer(YACHTROCK_DISCOVER_VERSION, &discover_err);
  if ( result == NULL ) {
    if ( errmsg ) {
      if ( discover_err == NULL ) {
        *errmsg = yr_strdup("discoverer returned NULL collection and didn't set error");
      } else {
        *errmsg = discover_err;
      }
    }
  }
 out:
  return result;
}

#endif

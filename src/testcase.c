#include <yachtrock/testcase.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

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
                                struct yr_suite_lifecycle_callbacks *callbacks,
                                const char *cs_names,
                                yr_test_case_function first, ...)
{
  size_t n_names = num_names(cs_names);
  size_t names_size = ginormous_namestring_size(cs_names);
  yr_test_suite_t suite = calloc(sizeof(yr_test_suite_s) + sizeof(yr_test_case_s) * n_names + names_size, 1);
  suite->name = name;
  if ( callbacks ) {
    suite->lifecycle = *callbacks;
  } else {
    suite->lifecycle = (struct yr_suite_lifecycle_callbacks){0};
  }
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

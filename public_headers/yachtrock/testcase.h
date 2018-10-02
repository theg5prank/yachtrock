#ifndef YACHTROCK_TESTCASE_H
#define YACHTROCK_TESTCASE_H

#include <yachtrock/base.h>

#include <stddef.h>

struct yr_test_suite;
typedef struct yr_test_suite yr_test_suite_s;
typedef yr_test_suite_s *yr_test_suite_t;
struct yr_test_case;
typedef struct yr_test_case yr_test_case_s;

typedef void (*yr_test_case_function)(yr_test_case_s testcase);
typedef void (*yr_test_case_setup_function)(yr_test_case_s testcase);
typedef void (*yr_test_case_teardown_function)(yr_test_case_s testcase);
typedef void (*yr_test_suite_setup_function)(yr_test_suite_t suite);
typedef void (*yr_test_suite_teardown_function)(yr_test_suite_t suite);

struct yr_test_case {
  const char *name;
  yr_test_case_function testcase;
  struct yr_test_suite *suite;
};

struct yr_suite_lifecycle_callbacks {
  yr_test_case_setup_function setup_case;
  yr_test_case_teardown_function teardown_case;
  yr_test_suite_setup_function setup_suite;
  yr_test_suite_teardown_function teardown_suite;
};

struct yr_test_suite {
  const char *name;
  void *refcon;
  struct yr_suite_lifecycle_callbacks lifecycle;
  size_t num_cases;
  yr_test_case_s cases[0];
};


YACHTROCK_EXTERN yr_test_suite_t
_yr_create_suite_from_functions(const char *name,
                                struct yr_suite_lifecycle_callbacks *callbacks,
                                const char *cs_names,
                                yr_test_case_function first, ...);
#define yr_create_suite_from_functions(name, lifecycle_callbacks, ...)  \
  _yr_create_suite_from_functions(name, lifecycle_callbacks, # __VA_ARGS__, __VA_ARGS__)

/* Create a blank suite on the heap that you have to fill out.
 * It has the right number of cases that have the suite pointers in the cases set correctly.
 * Everything else is zeroed out. This is the "escape hatch", really.
 */
YACHTROCK_EXTERN yr_test_suite_t
yr_create_blank_suite(size_t num_cases);

#endif
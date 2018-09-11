#ifndef YACHTROCK_YACHTROCK_H
#define YACHTROCK_YACHTROCK_H

#include <yachtrock/base.h>
#include <yachtrock/assert.h>
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

struct yr_test_suite {
  const char *name;
  void *refcon;
  yr_test_case_setup_function setup_case;
  yr_test_case_teardown_function teardown_case;
  yr_test_suite_setup_function setup_suite;
  yr_test_suite_teardown_function teardown_suite;
  size_t num_cases;
  yr_test_case_s cases[0];
};

YACHTROCK_EXTERN int yr_basic_run_suite(yr_test_suite_t suite);

#endif

#ifndef YACHTROCK_YACHTROCK_H
#define YACHTROCK_YACHTROCK_H

#include <stddef.h>
#include <yachtrock/base.h>
#include <yachtrock/assert.h>

struct yr_test_suite;
struct yr_test_case;

typedef void (*yr_test_case_function)(struct yr_test_case context);
/* typedef void (*yr_test_case_setup_function)(struct yr_test_case context); */
/* typedef void (*yr_test_case_teardown_function)(struct yr_test_case context); */

typedef struct yr_test_case {
  const char *name;
  void *refcon;
  yr_test_case_function testcase;
  struct yr_test_suite *suite;
} yr_test_case_s;
typedef yr_test_case_s *yr_test_case_t;

typedef struct yr_test_suite {
  const char *name;
  void *refcon;
  size_t num_cases;
  yr_test_case_s cases[0];
} yr_test_suite_s;
typedef yr_test_suite_s *yr_test_suite_t;

YACHTROCK_EXTERN int yr_basic_run_suite(yr_test_suite_t suite);

#endif

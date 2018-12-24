#ifndef YACHTROCK_TESTCASE_H
#define YACHTROCK_TESTCASE_H

#include <yachtrock/base.h>

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct yr_test_suite;
typedef struct yr_test_suite yr_test_suite_s;
typedef yr_test_suite_s *yr_test_suite_t;
struct yr_test_case;
typedef struct yr_test_case yr_test_case_s;
typedef const yr_test_case_s *yr_test_case_t;

typedef void (*yr_test_case_function)(yr_test_case_t testcase);
typedef void (*yr_test_case_setup_function)(yr_test_case_t testcase);
typedef void (*yr_test_case_teardown_function)(yr_test_case_t testcase);
typedef void (*yr_test_suite_setup_function)(yr_test_suite_t suite);
typedef void (*yr_test_suite_teardown_function)(yr_test_suite_t suite);

#define __YR_DEVARIADICIFY_2(dummy, A, ...) A
#define YR_TESTCASE(name, ...) void name(yr_test_case_t __YR_DEVARIADICIFY_2(dummy, ##__VA_ARGS__ , testcase) )

struct yr_test_case {
  const char *name;
  yr_test_case_function testcase;
  const struct yr_test_suite *suite;
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
  yr_test_case_s cases[];
};

YACHTROCK_EXTERN const struct yr_suite_lifecycle_callbacks YR_NO_CALLBACKS;

YACHTROCK_EXTERN yr_test_suite_t
_yr_create_suite_from_functions(const char *name,
                                void *suite_refcon,
                                struct yr_suite_lifecycle_callbacks callbacks,
                                const char *cs_names,
                                yr_test_case_function first, ...);
#define yr_create_suite_from_functions(name, suite_refcon, lifecycle_callbacks, ...) \
  _yr_create_suite_from_functions(name, suite_refcon, lifecycle_callbacks, \
                                  # __VA_ARGS__, __VA_ARGS__)

/* Create a blank suite on the heap that you have to fill out.
 * It has the right number of cases that have the suite pointers in the cases set correctly.
 * Everything else is zeroed out. This is the "escape hatch", really.
 */
YACHTROCK_EXTERN yr_test_suite_t
yr_create_blank_suite(size_t num_cases);

struct yr_test_suite_collection {
  size_t num_suites;
  // note: storage in the same allocation when returned from libyachtrock functions
  yr_test_suite_t suites[];
};
typedef struct yr_test_suite_collection yr_test_suite_collection_s;
typedef yr_test_suite_collection_s *yr_test_suite_collection_t;

YACHTROCK_EXTERN yr_test_suite_collection_t
yr_test_suite_collection_create_from_suites(size_t num_suites, yr_test_suite_t *suites);

YACHTROCK_EXTERN yr_test_suite_collection_t
yr_test_suite_collection_create_from_collections(size_t num_collections,
                                                 yr_test_suite_collection_t collection1,
                                                 ...);
YACHTROCK_EXTERN yr_test_suite_collection_t
yr_test_suite_collection_create_from_collection_array(size_t num_collections,
                                                       yr_test_suite_collection_t *collections);


#if YACHTROCK_DLOPEN

#define YACHTROCK_MODULE_DISCOVER_NAME yr_module_create_test_suite_collection

#define YACHTROCK_DISCOVER_VERSION 1

typedef yr_test_suite_collection_t
(*yr_module_discoverer_t)(unsigned discover_version, char **errmsg);

#define YACHTROCK_DEFINE_TEST_SUITE_COLLECTION_DISCOVERER()             \
  static yr_test_suite_collection_t                                     \
  YACHTROCK_MODULE_DISCOVER_NAME ## __impl(unsigned discover_version,   \
                                           char **errmsg);              \
  extern yr_test_suite_collection_t YACHTROCK_MODULE_DISCOVER_NAME(unsigned discover_version, \
                                                                   char **errmsg) \
  {                                                                     \
    if ( discover_version > YACHTROCK_DISCOVER_VERSION ) {              \
      char _;                                                           \
      char *fmt = "Discover version %d is greater than supported version %d"; \
      int required = snprintf(&_, 0, fmt, discover_version,             \
                              YACHTROCK_DISCOVER_VERSION) + 1;          \
      *errmsg = malloc(required);                                       \
      snprintf(*errmsg, required, fmt, discover_version,                \
               YACHTROCK_DISCOVER_VERSION);                             \
      return NULL;                                                      \
    }                                                                   \
    return (YACHTROCK_MODULE_DISCOVER_NAME ## __impl)(discover_version, \
                                                      errmsg);          \
  }                                                                     \
  static yr_test_suite_collection_t                                     \
  YACHTROCK_MODULE_DISCOVER_NAME ## __impl(unsigned discover_version,   \
                                           char **errmsg)


YACHTROCK_EXTERN yr_test_suite_collection_t
yr_test_suite_collection_create_from_dylib_path(const char *path, char **errmsg);

YACHTROCK_EXTERN yr_test_suite_collection_t
yr_test_suite_collection_create_from_handle(void *handle, char **errmsg);

#endif

#endif

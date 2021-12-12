#ifndef YACHTROCK_TESTCASE_H
#define YACHTROCK_TESTCASE_H

#include <yachtrock/base.h>

#include <stddef.h>

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

#define YR_DEVARIADICIFY_2(dummy, A, ...) A
/**
 * Helper macro to define a testcase function.
 */
#define YR_TESTCASE(name, ...) void name(yr_test_case_t YR_DEVARIADICIFY_2(dummy, ##__VA_ARGS__ , testcase) )

/**
 * A single test case.
 *
 * name: the name of the case.
 * testcase: the function that executes the test case.
 * suite: a pointer to the suite containing the test case.
 */
struct yr_test_case {
  const char *name;
  yr_test_case_function testcase;
  const struct yr_test_suite *suite;
};

/**
 * Callbacks for suite lifecycle events.
 *
 * setup_case: called when a testcase is about to be invoked.
 * teardown_case: called after a testcase is invoked.
 * setup_suite: called when a test suite is about to be invoked, before any testcases or testcase setup
 *              callbacks.
 * teardown_suite: called after a suite has been invoked.
 */
struct yr_suite_lifecycle_callbacks {
  yr_test_case_setup_function setup_case;
  yr_test_case_teardown_function teardown_case;
  yr_test_suite_setup_function setup_suite;
  yr_test_suite_teardown_function teardown_suite;
};

/**
 * A test suite.
 *
 * name: the name of the suite.
 * refcon: arbitrary pointer passed to suite lifecyle callbacks.
 * lifecycle: suite lifecycle callbacks.
 * num_cases: the number of cases.
 * cases: the case structures themselves, allocated inline.
 */
struct yr_test_suite {
  const char *name;
  void *refcon;
  struct yr_suite_lifecycle_callbacks lifecycle;
  size_t num_cases;
  yr_test_case_s cases[];
};

/**
 * A set of callbacks that do nothing.
 */
YACHTROCK_EXTERN const struct yr_suite_lifecycle_callbacks YR_NO_CALLBACKS;

YACHTROCK_EXTERN yr_test_suite_t
_yr_create_suite_from_functions(const char *name,
                                void *suite_refcon,
                                struct yr_suite_lifecycle_callbacks callbacks,
                                const char *cs_names,
                                yr_test_case_function first, ...);

/**
 * Create a suite from a static set of functions.
 *
 * The case names are taken from the function names. All storage consumed by the suite is in one
 * allocation and can and must be passed to free by the caller.
 */
#define yr_create_suite_from_functions(name, suite_refcon, lifecycle_callbacks, ...) \
  _yr_create_suite_from_functions(name, suite_refcon, lifecycle_callbacks, \
                                  # __VA_ARGS__, __VA_ARGS__)

/**
 * Create a blank suite on the heap that you have to fill out.
 *
 * The suite has the right number of cases that have the suite pointers in the cases set correctly.
 * Everything else is zeroed/nulled out. This is the "escape hatch", really.
 */
YACHTROCK_EXTERN yr_test_suite_t
yr_create_blank_suite(size_t num_cases);

/**
 * Create a blank suite on the heap that you have to fill out, with extra bytes as part of the
 * allocation.
 *
 * The suite has the right number of cases that have the suite pointers in the cases set correctly.
 * Everything else is zeroed/nulled out. This is the "escape hatch", really.
 */
YACHTROCK_EXTERN yr_test_suite_t
yr_create_blank_suite_with_extra_bytes(size_t num_cases, size_t extra_bytes, char **out_extra_bytes);

/**
 * A collection of test suites.
 *
 * num_suites: the number of suites.
 * suites: the suites. When created by libyachtrock functions, all storage in the suites is in the
 *         same allocation of the collection.
 */
struct yr_test_suite_collection {
  size_t num_suites;
  // note: storage in the same allocation when returned from libyachtrock functions
  yr_test_suite_t suites[];
};
typedef struct yr_test_suite_collection yr_test_suite_collection_s;
typedef yr_test_suite_collection_s *yr_test_suite_collection_t;

/**
 * Create a collection from an array of suites, copying the suites.
 *
 * The resulting collection contains all suite storage as well and must be passed to free by the
 * caller.
 */
YACHTROCK_EXTERN yr_test_suite_collection_t
yr_test_suite_collection_create_from_suites(size_t num_suites, yr_test_suite_t *suites);

/**
 * Create a collection from a set of collections, passed variadically.
 *
 * The resulting collection has copies of all suites in the input collections, containing all of
 * their storage, and must be passed to free by the caller.
 */
YACHTROCK_EXTERN yr_test_suite_collection_t
yr_test_suite_collection_create_from_collections(size_t num_collections,
                                                 yr_test_suite_collection_t collection1,
                                                 ...);

/**
 * Create a collection from a set of collections, passed as an array.
 *
 * The resulting collection has copies of all suites in the input collections, containing all of
 * their storage, and must be passed to free by the caller.
 */
YACHTROCK_EXTERN yr_test_suite_collection_t
yr_test_suite_collection_create_from_collection_array(size_t num_collections,
                                                       yr_test_suite_collection_t *collections);


#if YACHTROCK_DLOPEN

/**
 * The name of the test suite collection discovery function that test dylibs must export.
 */
#define YACHTROCK_MODULE_DISCOVER_NAME yr_module_create_test_suite_collection

#define YACHTROCK_DISCOVER_VERSION 1

typedef yr_test_suite_collection_t
(*yr_module_discoverer_t)(unsigned discover_version, char **errmsg);

YACHTROCK_EXTERN char *
_yr_create_version_mismatch_error(unsigned discover_version,
                                  unsigned yr_discover_version);

#ifdef __cplusplus
#define YACHTROCK_DISCOVERER_EXTERN extern "C"
#else
#define YACHTROCK_DISCOVERER_EXTERN extern
#endif

/**
 * Helper macro to define the test suite collection discovery function.
 */
#define YACHTROCK_DEFINE_TEST_SUITE_COLLECTION_DISCOVERER()             \
  static yr_test_suite_collection_t                                     \
  YACHTROCK_MODULE_DISCOVER_NAME ## __impl(unsigned discover_version,   \
                                           char **errmsg);              \
  YACHTROCK_DISCOVERER_EXTERN                                           \
  yr_test_suite_collection_t YACHTROCK_MODULE_DISCOVER_NAME(unsigned discover_version, \
                                                            char **errmsg) \
  {                                                                     \
    if ( discover_version > YACHTROCK_DISCOVER_VERSION ) {              \
      if ( errmsg ) {                                                   \
        *errmsg = _yr_create_version_mismatch_error(discover_version,   \
                                                    YACHTROCK_DISCOVER_VERSION); \
      }                                                                 \
      return NULL;                                                      \
    }                                                                   \
    return (YACHTROCK_MODULE_DISCOVER_NAME ## __impl)(discover_version, \
                                                      errmsg);          \
  }                                                                     \
  static yr_test_suite_collection_t                                     \
  YACHTROCK_MODULE_DISCOVER_NAME ## __impl(unsigned discover_version,   \
                                           char **errmsg)


/**
 * Discover a test suite collection provided by a dylib, by providing the path to the dylib.
 *
 * The resulting collection has all suite's storage and must be freed by the caller.
 */
YACHTROCK_EXTERN yr_test_suite_collection_t
yr_test_suite_collection_create_from_dylib_path(const char *path, char **errmsg);

/**
 * Discover a test suite collection provided by a dylib, by providing a handle for the dylib.
 *
 * The resulting collection has all suite's storage and must be freed by the caller.
 */
YACHTROCK_EXTERN yr_test_suite_collection_t
yr_test_suite_collection_create_from_handle(void *handle, char **errmsg);

#endif

#endif

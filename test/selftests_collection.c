#include "yrtests.h"

yr_test_suite_collection_t yachtrock_selftests_collection(void)
{
  yr_test_suite_t suites[] = {
    yr_create_basic_suite(),
    yr_create_result_store_suite(),
    yr_create_assertion_suite(),
    yr_create_testcase_suite(),
    yr_create_run_under_store_suite(),
#if YACHTROCK_MULTIPROCESS
    yr_create_multiprocess_suite(),
#endif
    yr_create_selector_suite(),
  };
  size_t suites_count = sizeof(suites)/sizeof(suites[0]);
  yr_test_suite_collection_t collection =
    yr_test_suite_collection_create_from_suites(suites_count, suites);
  for ( size_t i = 0; i < suites_count; i++ ) {
    free(suites[i]);
  }
  return collection;
}

#if YACHTROCK_DLOPEN
YACHTROCK_DEFINE_TEST_SUITE_COLLECTION_DISCOVERER()
{
  return yachtrock_selftests_collection();
}
#endif

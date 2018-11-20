#include <yachtrock/yachtrock.h>

YR_TESTCASE(case11) {}
YR_TESTCASE(case12) {}
YR_TESTCASE(case21) {}
YR_TESTCASE(case22) {}

YACHTROCK_DEFINE_TEST_SUITE_COLLECTION_DISCOVERER()
{
  yr_test_suite_t suite1 = yr_create_suite_from_functions("suite 1", NULL,
                                                          YR_NO_CALLBACKS,
                                                          case11, case12);
  yr_test_suite_t suite2 = yr_create_suite_from_functions("suite 2", NULL,
                                                          YR_NO_CALLBACKS,
                                                          case21, case22);
  yr_test_suite_t suites[] = {suite1, suite2};
  yr_test_suite_collection_t collection = yr_test_suite_collection_create_from_suites(2, suites);
  
  struct yr_test_suite_collection_discover_response response;
  response.version = YACHTROCK_DISCOVER_VERSION;
  response.collection = collection;

  return response;
}

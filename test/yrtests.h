#include <yachtrock/yachtrock.h>

extern yr_test_suite_t yr_create_basic_suite(void);
extern yr_test_suite_t yr_create_result_store_suite(void);
extern yr_test_suite_t yr_create_assertion_suite(void);
extern yr_test_suite_t yr_create_testcase_suite(void);
extern yr_test_suite_t yr_create_run_under_store_suite(void);
extern yr_test_suite_t yr_create_selector_suite(void);

#if YACHTROCK_MULTIPROCESS
extern yr_test_suite_t yr_create_multiprocess_suite(void);
#endif

#if YACHTROCK_HAS_SCRIPT_HELPERS
extern yr_test_suite_t yr_create_script_helpers_suite(void);
#endif

extern yr_test_suite_collection_t yachtrock_selftests_collection(void);

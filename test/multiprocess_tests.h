#include <yachtrock/yachtrock.h>

#define INFERIOR_COLLECTION_NAME_VAR "YR_MULTIPROCESS_TEST_COLLECTION_NAME"
#define TEST_TRAMPOLINE_PATH_VAR "YR_MULTIPROCESS_TEST_TRAMPOLINE"

extern yr_test_suite_collection_t
yr_multiprocess_test_inferior_create_collection(const char *collname);

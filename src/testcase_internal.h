#ifndef YACHTROCK_TESTCASE_INTERNAL_H
#define YACHTROCK_TESTCASE_INTERNAL_H

#include <yachtrock/testcase.h>

extern size_t test_suite_total_size_deconstructed(const char *suite_name, size_t num_cases,
                                                  char **case_names);
extern char *test_suite_string_storage_location(yr_test_suite_t suite);

#endif

#include <yachtrock/yachtrock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PASSING_TEST_CASE(name, fun, ...)       \
  static void name(yr_test_case_t tc)           \
  {                                             \
    fun(__VA_ARGS__);                           \
  }

#define FAILING_TEST_CASE(name, fun, ...)                               \
  static void __failing_test__ ## name(yr_test_case_t tc)               \
  {                                                                     \
    fun(__VA_ARGS__);                                                   \
  }                                                                     \
  static void name(yr_test_case_t tc)                                   \
  {                                                                     \
    yr_test_suite_t suite = yr_create_suite_from_functions(#name " (intentionally failing suite)", \
                                                           NULL, YR_NO_CALLBACKS, \
                                                           __failing_test__ ## name); \
    bool ok = yr_basic_run_suite(suite);                                \
    free(suite);                                                        \
    YR_ASSERT_FALSE(ok, "test should have failed!");                    \
  }

PASSING_TEST_CASE(test_assert_false, YR_ASSERT_FALSE, 1 == 2, "should not be equal");
FAILING_TEST_CASE(test_assert_false_fails, YR_ASSERT_FALSE, 1 == 1, "should be equal");

PASSING_TEST_CASE(test_assert_equal, YR_ASSERT_EQUAL, 1, 1, "should be equal (%d and %d)", 1, 2);
FAILING_TEST_CASE(test_assert_equal_fails, YR_ASSERT_EQUAL, 1, 2, "should not be equal (%d and %d)", 1, 2);

PASSING_TEST_CASE(test_assert_not_equal, YR_ASSERT_NOT_EQUAL, 4 * 5, 21, "should be not equal");
FAILING_TEST_CASE(test_assert_not_equal_fails, YR_ASSERT_NOT_EQUAL, 4 * 5, 20, "should not be not equal");

PASSING_TEST_CASE(test_assert_less_than, YR_ASSERT_LESS_THAN, 4, 5, "should be less than (%d and %d)", 4, 5);
FAILING_TEST_CASE(test_assert_less_than_fails, YR_ASSERT_LESS_THAN, 4, 4, "should not be less than (%d and %d)", 4, 4);

PASSING_TEST_CASE(test_assert_greater_than, YR_ASSERT_GREATER_THAN, 5, 4, "should be greater (%d and %d)", 5, 4);
FAILING_TEST_CASE(test_assert_greater_than_fails, YR_ASSERT_GREATER_THAN, 5, 5, "should not be greater than (%d and %d)", 5, 5);

PASSING_TEST_CASE(test_assert_less_than_equal_equal, YR_ASSERT_LESS_THAN_OR_EQUAL,
                  5, 5, "should be greater or equal (%d and %d)", 5, 5);
PASSING_TEST_CASE(test_assert_less_than_equal_less, YR_ASSERT_LESS_THAN_OR_EQUAL,
                  5, 10, "should be greater or equal (%d and %d)", 5, 10);
FAILING_TEST_CASE(test_assert_less_than_equal_fails,
                  YR_ASSERT_LESS_THAN_OR_EQUAL, 5, 4, "should not be greater than or equal (%d and %d)", 5, 4);

PASSING_TEST_CASE(test_assert_greater_than_equal_equal, YR_ASSERT_GREATER_THAN_OR_EQUAL,
                  5, 5, "should be greater or equal (%d and %d)", 5, 5);
PASSING_TEST_CASE(test_assert_greater_than_equal_greater, YR_ASSERT_GREATER_THAN_OR_EQUAL,
                  10, 5, "should be greater or equal (%d and %d)", 10, 5);
FAILING_TEST_CASE(test_assert_greater_than_equal_fails,
                  YR_ASSERT_GREATER_THAN_OR_EQUAL, 4, 5, "should not be greater than or equal (%d and %d)", 4, 5);

FAILING_TEST_CASE(test_fail, YR_FAIL, "failing is important sometimes: %s", "you betcha");

#define STRING_PASSING_TEST_CASE(name, macro, str1, str2)       \
  static void name(yr_test_case_t tc)                           \
  {                                                             \
    char duplicated1[strlen(str1) + 1];                         \
    strcpy(duplicated1, str1);                                  \
    char duplicated2[strlen(str2) + 1];                         \
    strcpy(duplicated2, str2);                                  \
    macro(str1, str2);                                          \
  }


STRING_PASSING_TEST_CASE(test_strings_equal_equal, YR_ASSERT_STRINGS_EQUAL, "hello", "hello")
FAILING_TEST_CASE(test_strings_equal_not_equal, YR_ASSERT_STRINGS_EQUAL, "hello", "there");

STRING_PASSING_TEST_CASE(test_strings_not_equal_not_equal, YR_ASSERT_STRINGS_NOT_EQUAL, "hello", "there");
FAILING_TEST_CASE(test_strings_not_equal_equal, YR_ASSERT_STRINGS_NOT_EQUAL, "hello", "hello");

yr_test_suite_t yr_create_assertion_suite(void)
{
  yr_test_suite_t suite = yr_create_suite_from_functions("assertion tests",
                                                         NULL, YR_NO_CALLBACKS,
                                                         test_assert_equal,
                                                         test_assert_equal_fails,
                                                         test_assert_false,
                                                         test_assert_false_fails,
                                                         test_assert_not_equal,
                                                         test_assert_not_equal_fails,
                                                         test_assert_less_than,
                                                         test_assert_less_than_fails,
                                                         test_assert_greater_than,
                                                         test_assert_greater_than_fails,
                                                         test_assert_less_than_equal_equal,
                                                         test_assert_less_than_equal_less,
                                                         test_assert_less_than_equal_fails,
                                                         test_assert_greater_than_equal_equal,
                                                         test_assert_greater_than_equal_greater,
                                                         test_assert_greater_than_equal_fails,
                                                         test_fail,
                                                         test_strings_equal_equal,
                                                         test_strings_equal_not_equal,
                                                         test_strings_not_equal_not_equal,
                                                         test_strings_not_equal_equal);
  return suite;
}

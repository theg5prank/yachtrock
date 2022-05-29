#ifndef YACHTROCK_ASSERT_H
#define YACHTROCK_ASSERT_H

#include <yachtrock/runtime.h>
#include <string.h>

/**
 * Basic assertion.
 */
#define YR_ASSERT(test, ...) do {                                       \
    if ( !(test) ) {                                                    \
      yr_fail_assertion(#test, __FILE__, __LINE__, __FUNCTION__, "" __VA_ARGS__); \
    }                                                                   \
  } while (0)

/**
 * Assertion that unconditionally fails.
 */
#define YR_FAIL(reason, ...) do {                                       \
    yr_fail_assertion("explicit failure", __FILE__, __LINE__, __FUNCTION__, reason, ##__VA_ARGS__); \
  } while (0)


/**
 * Assert that a condition does not hold.
 */
#define YR_ASSERT_FALSE(test, ...) do {                                 \
    if ( (test) ) {                                                     \
      yr_fail_assertion(#test, __FILE__, __LINE__, __FUNCTION__, "" __VA_ARGS__); \
    }                                                                   \
  } while (0)

/**
 * Assert that two values are equal.
 */
#define YR_ASSERT_EQUAL(val1, val2, ...) do {                           \
    if ( !((val1) == (val2)) ) {                                        \
      const char *__assertion_desc = #val1 " == " #val2;                \
      yr_fail_assertion(__assertion_desc, __FILE__, __LINE__, __FUNCTION__, "" __VA_ARGS__); \
    }                                                                   \
  } while (0)

/**
 * Assert that two values are not equal.
 */
#define YR_ASSERT_NOT_EQUAL(val1, val2, ...) do {                       \
    if ( !((val1) != (val2)) ) {                                        \
      const char *__assertion_desc = #val1 " != " #val2;                \
      yr_fail_assertion(__assertion_desc, __FILE__, __LINE__, __FUNCTION__, "" __VA_ARGS__); \
    }                                                                   \
  } while (0)

/**
 * Assert that one value is less than another (via the < operator).
 */
#define YR_ASSERT_LESS_THAN(val1, val2, ...) do {                       \
    if ( !((val1) < (val2)) ) {                                         \
      const char *__assertion_desc = #val1 " < " #val2;                 \
      yr_fail_assertion(__assertion_desc, __FILE__, __LINE__, __FUNCTION__, "" __VA_ARGS__); \
    }                                                                   \
  } while (0)

/**
 * Assert that one value is greater than another (via the > operator).
 */
#define YR_ASSERT_GREATER_THAN(val1, val2, ...) do {                    \
    if ( !((val1) > (val2)) ) {                                         \
      const char *__assertion_desc = #val1 " > " #val2;                 \
      yr_fail_assertion(__assertion_desc, __FILE__, __LINE__, __FUNCTION__, "" __VA_ARGS__); \
    }                                                                   \
  } while (0)

/**
 * Assert that one value is less than or equal to another (via the <= operator).
 */
#define YR_ASSERT_LESS_THAN_OR_EQUAL(val1, val2, ...) do {              \
    if ( !((val1) <= (val2)) ) {                                        \
      const char *__assertion_desc = #val1 " <= " #val2;                \
      yr_fail_assertion(__assertion_desc, __FILE__, __LINE__, __FUNCTION__, "" __VA_ARGS__); \
    }                                                                   \
  } while (0)

/**
 * Assert that one value is greater than or equal to another (via the >= operator).
 */
#define YR_ASSERT_GREATER_THAN_OR_EQUAL(val1, val2, ...) do {           \
    if ( !((val1) >= (val2)) ) {                                        \
      const char *__assertion_desc = #val1 " >= " #val2;                \
      yr_fail_assertion(__assertion_desc, __FILE__, __LINE__, __FUNCTION__, "" __VA_ARGS__); \
    }                                                                   \
  } while (0)

/**
 * Assert that two strings are equal.
 */
#define YR_ASSERT_STRINGS_EQUAL(val1, val2, ...) do {                   \
    if ( strcmp((val1), (val2)) != 0 ) {                                \
      const char *__assertion_desc = "strcmp("#val1 ", " #val2 ") == 0"; \
      yr_fail_assertion(__assertion_desc, __FILE__, __LINE__, __FUNCTION__, "" __VA_ARGS__); \
    }                                                                   \
  } while (0)

/**
 * Assert that two strings are not equal.
 */
#define YR_ASSERT_STRINGS_NOT_EQUAL(val1, val2, ...) do {               \
    if ( strcmp((val1), (val2)) == 0 ) {                                \
      const char *__assertion_desc = "strcmp("#val1 ", " #val2 ") != 0"; \
      yr_fail_assertion(__assertion_desc, __FILE__, __LINE__, __FUNCTION__, "" __VA_ARGS__); \
    }                                                                   \
  } while (0)

#endif

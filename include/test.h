#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>

#define TEST_ASSERT_IMPL(condition, report_failure)                           \
    do {                                                                      \
        if (!(condition)) {                                                   \
            if (report_failure) {                                             \
                fprintf(stderr, "%s:%d: assertion failed: %s\n",             \
                        __FILE__, __LINE__, #condition);                      \
            }                                                                 \
            return EXIT_FAILURE;                                              \
        }                                                                     \
    } while (0)

#define TEST_ASSERT(condition) TEST_ASSERT_IMPL(condition, 1)
#define TEST_ASSERT_QUIET(condition) TEST_ASSERT_IMPL(condition, 0)

#define TEST_ASSERT_EQ_INT(expected, actual)                                  \
    do {                                                                      \
        const int test_expected_value = (expected);                           \
        const int test_actual_value = (actual);                               \
        if (test_expected_value != test_actual_value) {                       \
            fprintf(stderr,                                                   \
                    "%s:%d: expected %d, received %d\n",                     \
                    __FILE__, __LINE__, test_expected_value,                  \
                    test_actual_value);                                       \
            return EXIT_FAILURE;                                              \
        }                                                                     \
    } while (0)

#endif

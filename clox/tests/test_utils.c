#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define TEST(test_name) void test_##test_name()
#define RUN_TEST(test_name) run_test(test_##test_name, #test_name)

// Macro for basic assertion
#define ASSERT(condition) do { \
    if (!(condition)) { \
        printf(ANSI_COLOR_RED "FAILED" ANSI_COLOR_RESET ": %s\n", "Assertion failed: " #condition); \
        return; \
    } \
} while (0)

// Macro for asserting equality
#define ASSERT_EQUAL(expected, actual) do { \
    if ((expected) != (actual)) { \
        printf(ANSI_COLOR_RED "FAILED" ANSI_COLOR_RESET ": %s != %s\n", #expected, #actual); \
        return; \
    } \
} while (0)

// Macro for asserting string equality
#define ASSERT_STRING_EQUAL(expected, actual) do { \
    if (strcmp((expected), (actual)) != 0) { \
        printf(ANSI_COLOR_RED "FAILED" ANSI_COLOR_RESET ": \"%s\" != \"%s\"\n", expected, actual); \
        return; \
    } \
} while (0)

// Macro for asserting floating point equality with epsilon
#define ASSERT_FLOAT_EQUAL(expected, actual, epsilon) do { \
    if (fabs((expected) - (actual)) > (epsilon)) { \
        printf(ANSI_COLOR_RED "FAILED" ANSI_COLOR_RESET ": %f != %f (within %f)\n", (double)(expected), (double)(actual), (double)(epsilon)); \
        return; \
    } \
} while (0)

// Function to run a test
void run_test(void (*test_func)(), const char* test_name) {
    printf("Testing %s ... ", test_name);
    fflush(stdout);
    test_func();
    printf(ANSI_COLOR_GREEN "Passed\n" ANSI_COLOR_RESET);
}

#endif // TEST_UTILS_H


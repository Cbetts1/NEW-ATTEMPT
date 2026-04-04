/* =============================================================================
 * AI Aura OS — Lightweight Host-Side Test Framework
 *
 * Usage:
 *   #include "tests/framework.h"
 *
 *   static void test_foo(void) {
 *       TEST_ASSERT(1 + 1 == 2);
 *       TEST_ASSERT_EQ(foo(), 42);
 *   }
 *
 *   int main(void) {
 *       printf("=== my_suite ===\n");
 *       RUN_TEST(test_foo);
 *       PRINT_RESULTS();
 *   }
 * =============================================================================*/

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>

static int g_tests_run    = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

/* Basic boolean assertion */
#define TEST_ASSERT(cond) do {                                              \
    g_tests_run++;                                                          \
    if (!(cond)) {                                                          \
        fprintf(stderr, "    FAIL  [%s:%d]  %s\n",                         \
                __FILE__, __LINE__, #cond);                                 \
        g_tests_failed++;                                                   \
    } else {                                                                \
        g_tests_passed++;                                                   \
    }                                                                       \
} while (0)

/* Equality assertion (integral or pointer) */
#define TEST_ASSERT_EQ(a, b) do {                                           \
    g_tests_run++;                                                          \
    if ((a) != (b)) {                                                       \
        fprintf(stderr, "    FAIL  [%s:%d]  (%s) == (%s)\n",               \
                __FILE__, __LINE__, #a, #b);                                \
        g_tests_failed++;                                                   \
    } else {                                                                \
        g_tests_passed++;                                                   \
    }                                                                       \
} while (0)

/* NULL / non-NULL helpers */
#define TEST_ASSERT_NULL(p)     TEST_ASSERT((p) == NULL)
#define TEST_ASSERT_NOT_NULL(p) TEST_ASSERT((p) != NULL)

/* Run a single test function, printing its name */
#define RUN_TEST(fn) do {                                                   \
    int _before_fail = g_tests_failed;                                      \
    fn();                                                                   \
    if (g_tests_failed == _before_fail)                                     \
        printf("  PASS  " #fn "\n");                                        \
    else                                                                    \
        printf("  FAIL  " #fn "\n");                                        \
} while (0)

/* Print summary and return appropriate exit code — put at end of main() */
#define PRINT_RESULTS() do {                                                \
    printf("\n  %d tests run | %d passed | %d failed\n",                   \
           g_tests_run, g_tests_passed, g_tests_failed);                    \
    return (g_tests_failed > 0) ? 1 : 0;                                   \
} while (0)

#endif /* TEST_FRAMEWORK_H */

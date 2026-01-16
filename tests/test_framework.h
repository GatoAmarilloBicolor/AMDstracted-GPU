/*
 * Test Framework - Simple but powerful testing for AMDGPU_Abstracted
 * 
 * Inspired by: CUnit, cmocka, Google Test
 * Simplified for portability and clarity
 * 
 * Developed by: Haiku Imposible Team (HIT)
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

/* ============================================================================
 * Test Statistics Tracking
 * ============================================================================ */

typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    int skipped_tests;
    long long total_time_ms;
} test_stats_t;

static test_stats_t g_test_stats = {0, 0, 0, 0, 0};

/* ============================================================================
 * Assertion Macros (Core Testing)
 * ============================================================================ */

#define TEST_ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "[FAIL] %s:%d - Assertion failed: %s\n", \
                   __FILE__, __LINE__, #condition); \
            return 0; \
        } \
    } while (0)

#define TEST_ASSERT_FALSE(condition) \
    TEST_ASSERT_TRUE(!(condition))

#define TEST_ASSERT_EQUAL_INT(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            fprintf(stderr, "[FAIL] %s:%d - Expected %d but got %d\n", \
                   __FILE__, __LINE__, expected, actual); \
            return 0; \
        } \
    } while (0)

#define TEST_ASSERT_EQUAL_PTR(expected, actual) \
    do { \
        if ((void*)(expected) != (void*)(actual)) { \
            fprintf(stderr, "[FAIL] %s:%d - Expected %p but got %p\n", \
                   __FILE__, __LINE__, (void*)(expected), (void*)(actual)); \
            return 0; \
        } \
    } while (0)

#define TEST_ASSERT_NULL(ptr) \
    TEST_ASSERT_EQUAL_PTR(NULL, (ptr))

#define TEST_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            fprintf(stderr, "[FAIL] %s:%d - Pointer is NULL\n", \
                   __FILE__, __LINE__); \
            return 0; \
        } \
    } while (0)

#define TEST_ASSERT_EQUAL_STR(expected, actual) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            fprintf(stderr, "[FAIL] %s:%d - Expected \"%s\" but got \"%s\"\n", \
                   __FILE__, __LINE__, (expected), (actual)); \
            return 0; \
        } \
    } while (0)

#define TEST_ASSERT_EQUAL_MEM(expected, actual, size) \
    do { \
        if (memcmp((expected), (actual), (size)) != 0) { \
            fprintf(stderr, "[FAIL] %s:%d - Memory mismatch\n", \
                   __FILE__, __LINE__); \
            return 0; \
        } \
    } while (0)

#define TEST_ASSERT_IN_RANGE(value, min, max) \
    do { \
        if ((value) < (min) || (value) > (max)) { \
            fprintf(stderr, "[FAIL] %s:%d - Value %d not in range [%d, %d]\n", \
                   __FILE__, __LINE__, value, min, max); \
            return 0; \
        } \
    } while (0)

/* ============================================================================
 * Test Lifecycle Macros
 * ============================================================================ */

#define TEST_SETUP() \
    static int test_setup(void)

#define TEST_TEARDOWN() \
    static int test_teardown(void)

#define TEST_CASE(name) \
    static int test_##name(void)

#define TEST_SKIP(reason) \
    do { \
        fprintf(stderr, "[SKIP] %s - %s\n", __func__, (reason)); \
        return -1; \
    } while (0)

/* ============================================================================
 * Test Execution Framework
 * ============================================================================ */

typedef struct {
    const char *name;
    int (*func)(void);
} test_entry_t;

#define TEST_REGISTER(test_name) \
    {#test_name, test_##test_name}

#define TEST_REGISTER_END \
    {NULL, NULL}

/*
 * Run a single test
 * Returns: 1=pass, 0=fail, -1=skip
 */
static inline int run_test(const char *test_name, int (*test_func)(void))
{
    if (!test_func) return -1;
    
    g_test_stats.total_tests++;
    
    int result = test_func();
    
    if (result == 1) {
        g_test_stats.passed_tests++;
        printf("[PASS] %s\n", test_name);
        return 1;
    } else if (result == -1) {
        g_test_stats.skipped_tests++;
        return -1;
    } else {
        g_test_stats.failed_tests++;
        return 0;
    }
}

/*
 * Run all tests in an array
 * Returns: number of failed tests
 */
static inline int run_all_tests(test_entry_t *tests)
{
    if (!tests) return -1;
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║          AMDGPU_Abstracted Test Suite                      ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    for (int i = 0; tests[i].name != NULL; i++) {
        run_test(tests[i].name, tests[i].func);
    }
    
    // Print summary
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║                    Test Summary                             ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║ Total:    %3d                                              ║\n", g_test_stats.total_tests);
    printf("║ Passed:   %3d ✅                                          ║\n", g_test_stats.passed_tests);
    printf("║ Failed:   %3d ❌                                          ║\n", g_test_stats.failed_tests);
    printf("║ Skipped:  %3d ⏭️                                          ║\n", g_test_stats.skipped_tests);
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    if (g_test_stats.failed_tests == 0) {
        printf("✅ ALL TESTS PASSED!\n\n");
        return 0;
    } else {
        printf("❌ %d tests failed\n\n", g_test_stats.failed_tests);
        return g_test_stats.failed_tests;
    }
}

/* ============================================================================
 * Test Helpers & Utilities
 * ============================================================================ */

/*
 * Get current time in milliseconds
 */
static inline long long get_time_ms(void)
{
    #ifdef __HAIKU__
        // TODO: Haiku timing
        return 0;
    #else
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    #endif
}

/*
 * Simple memory leak detector
 * Track allocations for assertion at test end
 */
typedef struct {
    void *ptr;
    size_t size;
    const char *location;
} alloc_tracker_t;

#define MAX_TRACKED_ALLOCATIONS 100
static alloc_tracker_t g_allocations[MAX_TRACKED_ALLOCATIONS];
static int g_alloc_count = 0;

static inline void *tracked_malloc(size_t size, const char *file, int line __attribute__((unused)))
{
    void *ptr = malloc(size);
    if (ptr && g_alloc_count < MAX_TRACKED_ALLOCATIONS) {
        g_allocations[g_alloc_count].ptr = ptr;
        g_allocations[g_alloc_count].size = size;
        g_allocations[g_alloc_count].location = file;
        g_alloc_count++;
    }
    return ptr;
}

static inline void tracked_free(void *ptr)
{
    if (!ptr) return;
    
    for (int i = 0; i < g_alloc_count; i++) {
        if (g_allocations[i].ptr == ptr) {
            g_allocations[i].ptr = NULL;
            break;
        }
    }
    free(ptr);
}

static inline int check_memory_leaks(void)
{
    int leaks = 0;
    for (int i = 0; i < g_alloc_count; i++) {
        if (g_allocations[i].ptr != NULL) {
            fprintf(stderr, "[LEAK] %zu bytes at %s\n", 
                   g_allocations[i].size, g_allocations[i].location);
            leaks++;
            tracked_free(g_allocations[i].ptr);
        }
    }
    g_alloc_count = 0;
    return leaks;
}

#define TEST_MALLOC(size) tracked_malloc((size), __FILE__, __LINE__)
#define TEST_FREE(ptr) tracked_free(ptr)
#define TEST_CHECK_LEAKS() check_memory_leaks()

#endif // TEST_FRAMEWORK_H

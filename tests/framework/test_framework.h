#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Test framework macros and functions
#define TEST_PASS 0
#define TEST_FAIL 1

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("❌ ASSERT FAILED: %s (%s:%d)\n", message, __FILE__, __LINE__); \
            return TEST_FAIL; \
        } \
    } while (0)

#define TEST_ASSERT_EQUAL(a, b, message) \
    do { \
        if ((a) != (b)) { \
            printf("❌ ASSERT EQUAL FAILED: %s (expected %d, got %d) (%s:%d)\n", message, (int)(a), (int)(b), __FILE__, __LINE__); \
            return TEST_FAIL; \
        } \
    } while (0)

#define TEST_ASSERT_NOT_NULL(ptr, message) \
    TEST_ASSERT((ptr) != NULL, message)

#define TEST_ASSERT_NULL(ptr, message) \
    TEST_ASSERT((ptr) == NULL, message)

// Test suite structure
typedef struct {
    const char *name;
    int (*setup)(void);
    int (*teardown)(void);
    int (*run)(void);
} test_suite_t;

// Test runner functions
int test_run_suite(const test_suite_t *suite);
int test_run_all_suites(const test_suite_t **suites, int count);

// Mock framework for OS interfaces
typedef struct {
    void *(*alloc_mock)(size_t size);
    void (*free_mock)(void *ptr);
    void (*log_mock)(const char *fmt, ...);
    // Add more mocks as needed
} os_mock_t;

// Global mock instance
extern os_mock_t *global_os_mock;

// Mock setup functions
void mock_os_setup(void);
void mock_os_teardown(void);

// Mock implementations
void *mock_alloc(size_t size);
void mock_free(void *ptr);
void mock_log(const char *fmt, ...);

// Coverage tracking (basic)
typedef struct {
    int total_lines;
    int covered_lines;
    const char *file;
} coverage_info_t;

void coverage_start_tracking(const char *file);
void coverage_record_line(int line);
void coverage_report(void);

// Performance testing
typedef struct {
    uint64_t start_time;
    uint64_t end_time;
} perf_timer_t;

void perf_timer_start(perf_timer_t *timer);
void perf_timer_stop(perf_timer_t *timer);
uint64_t perf_timer_elapsed_ns(perf_timer_t *timer);

#endif // TEST_FRAMEWORK_H
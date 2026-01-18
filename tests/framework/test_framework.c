#include "test_framework.h"
#include <time.h>
#include <stdarg.h>
#include <stdint.h>

// Global mock instance
os_mock_t *global_os_mock = NULL;

// Mock setup
void mock_os_setup(void) {
    global_os_mock = calloc(1, sizeof(os_mock_t));
    global_os_mock->alloc_mock = mock_alloc;
    global_os_mock->free_mock = mock_free;
    global_os_mock->log_mock = mock_log;
}

void mock_os_teardown(void) {
    free(global_os_mock);
    global_os_mock = NULL;
}

// Mock implementations
void *mock_alloc(size_t size) {
    return calloc(1, size); // Zero-initialized for testing
}

void mock_free(void *ptr) {
    free(ptr);
}

void mock_log(const char *fmt, ...) {
    // Silent logging for tests, or redirect to test output
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args); // Or suppress for clean output
    va_end(args);
}

// Test runner
int test_run_suite(const test_suite_t *suite) {
    printf("Running suite: %s\n", suite->name);

    if (suite->setup) {
        if (suite->setup() != 0) {
            printf("❌ Setup failed for %s\n", suite->name);
            return TEST_FAIL;
        }
    }

    int result = suite->run();

    if (suite->teardown) {
        suite->teardown();
    }

    printf("Suite %s: %s\n", suite->name, result == TEST_PASS ? "✅ PASS" : "❌ FAIL");
    return result;
}

int test_run_all_suites(const test_suite_t **suites, int count) {
    int passed = 0;
    for (int i = 0; i < count; i++) {
        if (test_run_suite(suites[i]) == TEST_PASS) {
            passed++;
        }
    }
    printf("\nOverall: %d/%d suites passed\n", passed, count);
    return passed == count ? TEST_PASS : TEST_FAIL;
}

// Basic coverage tracking
static coverage_info_t *coverage_data = NULL;
static int coverage_count = 0;

void coverage_start_tracking(const char *file) {
    coverage_count++;
    coverage_data = realloc(coverage_data, coverage_count * sizeof(coverage_info_t));
    coverage_data[coverage_count - 1].file = file;
    coverage_data[coverage_count - 1].total_lines = 0;
    coverage_data[coverage_count - 1].covered_lines = 0;
}

void coverage_record_line(int line) {
    // Simple implementation - in real coverage, track executed lines
    if (coverage_count > 0) {
        coverage_data[coverage_count - 1].covered_lines++;
    }
}

void coverage_report(void) {
    printf("\nCoverage Report:\n");
    for (int i = 0; i < coverage_count; i++) {
        printf("  %s: %d/%d lines covered\n",
               coverage_data[i].file,
               coverage_data[i].covered_lines,
               coverage_data[i].total_lines);
    }
    free(coverage_data);
    coverage_data = NULL;
    coverage_count = 0;
}

// Performance timing
void perf_timer_start(perf_timer_t *timer) {
    // Simple implementation using clock()
    timer->start_time = clock();
}

void perf_timer_stop(perf_timer_t *timer) {
    timer->end_time = clock();
}

uint64_t perf_timer_elapsed_ns(perf_timer_t *timer) {
    return ((timer->end_time - timer->start_time) * 1000000000ULL) / CLOCKS_PER_SEC;
}
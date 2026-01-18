#include "../framework/test_framework.h"
#include "../../os/os_interface.h"

// Mock test suite
static int mock_test_setup(void) {
    mock_os_setup();
    return TEST_PASS;
}

static int mock_test_teardown(void) {
    mock_os_teardown();
    return TEST_PASS;
}

static int test_mock_alloc_free(void) {
    void *ptr = global_os_mock->alloc_mock(64);
    TEST_ASSERT_NOT_NULL(ptr, "Mock alloc should return valid pointer");

    global_os_mock->free_mock(ptr);
    return TEST_PASS;
}

static int test_mock_log(void) {
    // Test that log doesn't crash
    global_os_mock->log_mock("Test log message");
    return TEST_PASS;
}

static int test_os_interface_calls(void) {
    // Test that OS interface functions are properly linked
    struct os_interface *os_if = os_get_interface();
    TEST_ASSERT_NOT_NULL(os_if, "OS interface should be available");

    void *ptr = os_if->alloc(32);
    TEST_ASSERT_NOT_NULL(ptr, "OS alloc should work");
    os_if->free(ptr);

    return TEST_PASS;
}

// Performance test
static int test_performance(void) {
    perf_timer_t timer;
    perf_timer_start(&timer);

    // Simulate some work
    for (int i = 0; i < 1000; i++) {
        void *ptr = global_os_mock->alloc_mock(16);
        global_os_mock->free_mock(ptr);
    }

    perf_timer_stop(&timer);
    uint64_t elapsed = perf_timer_elapsed_ns(&timer);

    printf("Performance: %lu ns for 1000 alloc/free cycles\n", elapsed);
    TEST_ASSERT(elapsed > 0, "Elapsed time should be positive");

    return TEST_PASS;
}

// Test suite definition
const test_suite_t mock_test_suite = {
    .name = "Mock OS Interface Tests",
    .setup = mock_test_setup,
    .teardown = mock_test_teardown,
    .run = NULL  // Will be set to run individual tests
};

// Individual tests array
typedef int (*test_fn)(void);
static test_fn mock_tests[] = {
    test_mock_alloc_free,
    test_mock_log,
    test_os_interface_calls,
    test_performance,
    NULL
};

// Main test runner for mocks
int run_mock_tests(void) {
    int passed = 0;
    int total = 0;

    for (int i = 0; mock_tests[i] != NULL; i++) {
        printf("Running mock test %d...\n", i + 1);
        if (mock_tests[i]() == TEST_PASS) {
            passed++;
        }
        total++;
    }

    printf("Mock tests: %d/%d passed\n", passed, total);
    return passed == total ? TEST_PASS : TEST_FAIL;
}
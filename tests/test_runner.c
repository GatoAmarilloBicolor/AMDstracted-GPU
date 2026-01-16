/*
 * Test Runner - Main entry point for all tests
 * 
 * Runs all test suites and generates reports
 * 
 * Developed by: Haiku Imposible Team (HIT)
 */

#include "test_framework.h"
#include <time.h>

/* Forward declare test suites */
extern test_entry_t gmc_v10_tests[];

/* ============================================================================
 * Test Suite Registry
 * ============================================================================ */

typedef struct {
    const char *suite_name;
    test_entry_t *tests;
} test_suite_t;

test_suite_t all_suites[] = {
    {"GMC v10 (Memory Controller)", gmc_v10_tests},
    {NULL, NULL}  // Terminator
};

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

int main(int argc, char *argv[])
{
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║                                                               ║\n");
    printf("║        AMDGPU_Abstracted - Unit Test Suite                  ║\n");
    printf("║        Haiku Imposible Team (HIT)                           ║\n");
    printf("║                                                               ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    long long start_time = get_time_ms();
    
    int total_failed = 0;
    
    // Run each test suite
    for (int i = 0; all_suites[i].suite_name != NULL; i++) {
        printf("\n");
        printf("════════════════════════════════════════════════════════════════\n");
        printf("Suite: %s\n", all_suites[i].suite_name);
        printf("════════════════════════════════════════════════════════════════\n");
        printf("\n");
        
        total_failed += run_all_tests(all_suites[i].tests);
        
        printf("\n");
    }
    
    long long end_time = get_time_ms();
    g_test_stats.total_time_ms = end_time - start_time;
    
    // Final summary
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║                    OVERALL TEST SUMMARY                       ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║ Total Tests:    %d                                           ║\n", g_test_stats.total_tests);
    printf("║ Passed:         %d ✅                                       ║\n", g_test_stats.passed_tests);
    printf("║ Failed:         %d ❌                                       ║\n", g_test_stats.failed_tests);
    printf("║ Skipped:        %d ⏭️                                        ║\n", g_test_stats.skipped_tests);
    printf("║ Execution Time: %lld ms                                   ║\n", g_test_stats.total_time_ms);
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    
    if (g_test_stats.failed_tests == 0) {
        printf("║ Status:         ✅ ALL TESTS PASSED                            ║\n");
        printf("╚═══════════════════════════════════════════════════════════════╝\n");
        printf("\n🎉 SUCCESS: All tests passed!\n\n");
        return 0;
    } else {
        printf("║ Status:         ❌ %d TESTS FAILED                         ║\n", 
               g_test_stats.failed_tests);
        printf("╚═══════════════════════════════════════════════════════════════╝\n");
        printf("\n❌ FAILURE: Some tests failed.\n\n");
        return 1;
    }
}

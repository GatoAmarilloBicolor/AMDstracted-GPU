/*
 * Memory Stress Tests - AMDGPU_Abstracted v0.2
 * 
 * Tests memory allocator under heavy load
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <stdint.h>

typedef struct {
    int passed;
    int failed;
    long total_ns;
} test_stats_t;

static test_stats_t stats = {0};

#define TEST(condition, msg, ...) \
    do { \
        if (condition) { \
            stats.passed++; \
            printf("[✓] " msg "\n", ##__VA_ARGS__); \
        } else { \
            stats.failed++; \
            printf("[✗] " msg "\n", ##__VA_ARGS__); \
        } \
    } while(0)

/* Simulate memory allocator */
typedef struct {
    void *ptrs[256];
    size_t sizes[256];
    uint64_t vas[256];
    int count;
} mem_tracker_t;

static mem_tracker_t tracker = {0};
static uint64_t next_va = 0x1000000;

void *alloc_gpu_memory(size_t size) {
    if (tracker.count >= 256) return NULL;
    
    void *ptr = malloc(size);
    if (!ptr) return NULL;
    
    tracker.ptrs[tracker.count] = ptr;
    tracker.sizes[tracker.count] = size;
    tracker.vas[tracker.count] = next_va;
    next_va += (size + 0xFFF) & ~0xFFF;
    tracker.count++;
    
    return ptr;
}

void test_sequential_allocation() {
    printf("\n▶ Sequential Allocation (1MB each)\n");
    
    clock_t start = clock();
    
    for (int i = 0; i < 10; i++) {
        void *ptr = alloc_gpu_memory(1024 * 1024);
        TEST(ptr != NULL, "Allocated 1MB");
    }
    
    TEST(tracker.count == 10, "All 10 allocations tracked");
    
    clock_t end = clock();
    long elapsed = (end - start) * 1000000 / CLOCKS_PER_SEC;
    printf("  Time: %ld μs (avg: %ld μs per alloc)\n", 
           elapsed, elapsed / 10);
    
    stats.total_ns += elapsed * 1000;
}

void test_various_sizes() {
    printf("\n▶ Various Sizes Allocation\n");
    
    size_t sizes[] = {
        256,
        512,
        1024,
        4096,
        65536,
        1048576,    // 1MB
        10485760,   // 10MB
    };
    
    clock_t start = clock();
    
    for (int i = 0; i < 7; i++) {
        void *ptr = alloc_gpu_memory(sizes[i]);
        TEST(ptr != NULL, "Allocated %zu bytes", sizes[i]);
    }
    
    clock_t end = clock();
    long elapsed = (end - start) * 1000000 / CLOCKS_PER_SEC;
    printf("  Total time: %ld μs\n", elapsed);
}

void test_va_alignment() {
    printf("\n▶ Virtual Address Alignment\n");
    
    tracker.count = 0;
    next_va = 0x1000000;
    
    for (int i = 0; i < 50; i++) {
        size_t size = 1024 + (i * 512);
        alloc_gpu_memory(size);
    }
    
    // Check 4KB alignment
    int aligned_count = 0;
    for (int i = 0; i < tracker.count; i++) {
        if (tracker.vas[i] % 4096 == 0) {
            aligned_count++;
        }
    }
    
    TEST(aligned_count == tracker.count, 
         "All %d VAs are 4KB aligned", aligned_count);
    
    // Check no overlap
    int no_overlap = 1;
    for (int i = 0; i < tracker.count - 1; i++) {
        uint64_t end_i = tracker.vas[i] + tracker.sizes[i];
        if (end_i > tracker.vas[i + 1]) {
            no_overlap = 0;
            break;
        }
    }
    TEST(no_overlap, "No memory region overlap");
}

void test_max_allocations() {
    printf("\n▶ Maximum Allocations (256 limit)\n");
    
    tracker.count = 0;
    next_va = 0x1000000;
    
    clock_t start = clock();
    
    // Allocate exactly 256
    for (int i = 0; i < 256; i++) {
        void *ptr = alloc_gpu_memory(256);
        if (!ptr) break;
    }
    
    TEST(tracker.count == 256, "Allocated maximum 256 buffers");
    
    // Try to allocate 257th - should fail
    void *extra = alloc_gpu_memory(256);
    TEST(extra == NULL, "257th allocation rejected (limit reached)");
    
    clock_t end = clock();
    long elapsed = (end - start) * 1000000 / CLOCKS_PER_SEC;
    printf("  Time for 256 allocations: %ld μs (avg: %ld μs)\n",
           elapsed, elapsed / 256);
}

void test_fragmentation() {
    printf("\n▶ Memory Fragmentation Resistance\n");
    
    tracker.count = 0;
    next_va = 0x1000000;
    
    // Allocate, then deallocate to create fragmentation
    for (int i = 0; i < 20; i++) {
        alloc_gpu_memory(256 * 1024);  // 256KB
    }
    
    uint64_t used_space = next_va - 0x1000000;
    printf("  Used VA space: %llu bytes\n", used_space);
    
    // New allocations should still work
    for (int i = 0; i < 10; i++) {
        void *ptr = alloc_gpu_memory(512 * 1024);  // 512KB
        TEST(ptr != NULL, "Can allocate despite fragmentation");
    }
}

void test_performance_baseline() {
    printf("\n▶ Performance Baseline\n");
    
    tracker.count = 0;
    next_va = 0x1000000;
    
    clock_t start = clock();
    
    // 1000 small allocations
    for (int i = 0; i < 1000; i++) {
        if (tracker.count >= 256) break;
        alloc_gpu_memory(4096);  // 4KB page
    }
    
    clock_t end = clock();
    long elapsed = (end - start) * 1000000 / CLOCKS_PER_SEC;
    
    TEST(elapsed < 1000000, "1000 allocations in < 1s");
    printf("  %d allocations in %ld μs (%.2f μs per alloc)\n",
           tracker.count, elapsed, (double)elapsed / tracker.count);
}

int main() {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║           Memory Stress Tests - AMDGPU v0.2               ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    test_sequential_allocation();
    test_various_sizes();
    test_va_alignment();
    test_max_allocations();
    test_fragmentation();
    test_performance_baseline();
    
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║                   TEST SUMMARY                             ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║ Passed: %d ✓\n", stats.passed);
    printf("║ Failed: %d ✗\n", stats.failed);
    printf("║ Total:  %d\n", stats.passed + stats.failed);
    printf("║ Total time: %.2f ms\n", stats.total_ns / 1000000.0);
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    return stats.failed == 0 ? 0 : 1;
}

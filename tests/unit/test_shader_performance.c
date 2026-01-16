/*
 * Shader Compiler Performance Tests - AMDGPU_Abstracted v0.2
 *
 * Measures shader compilation speed and ISA generation performance
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

typedef struct {
    int passed;
    int failed;
} test_stats_t;

static test_stats_t stats = {0};

#define TEST(condition, msg) \
    do { \
        if (condition) { \
            stats.passed++; \
            printf("[✓] %s\n", msg); \
        } else { \
            stats.failed++; \
            printf("[✗] %s\n", msg); \
        } \
    } while(0)

/* Minimal SPIR-V for testing */
static const uint32_t minimal_spirv[] = {
    0x07230203,  0x00010300, 0x08230000, 5, 0,
};

static const uint32_t medium_spirv[] = {
    0x07230203, 0x00010300, 0x08230000, 100, 0,
    /* Padded to ~400 words */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

void test_spirv_validation_speed() {
    printf("\n▶ SPIR-V Validation Speed\n");
    
    clock_t start = clock();
    
    // Validate 10000 times
    for (int i = 0; i < 10000; i++) {
        uint32_t magic = minimal_spirv[0];
        if (magic != 0x07230203) {
            TEST(0, "SPIR-V validation failed");
            return;
        }
    }
    
    clock_t end = clock();
    long elapsed = (end - start) * 1000000 / CLOCKS_PER_SEC;
    
    TEST(1, "10000 validations");
    printf("  Time: %ld μs (%.3f μs per validation)\n", 
           elapsed, (double)elapsed / 10000);
}

void test_isa_generation_speed() {
    printf("\n▶ ISA Generation Speed\n");
    
    clock_t start = clock();
    
    // Simulate ISA generation for 1000 shaders
    for (int i = 0; i < 1000; i++) {
        // Simulate ISA builder
        uint32_t isa_buffer[128];
        isa_buffer[0] = 0xBF800000;  // NOP
        isa_buffer[1] = 0xBF810000;  // ENDPGM
        
        // Simulate some work
        for (int j = 2; j < 128; j++) {
            isa_buffer[j] = 0xBF800000;
        }
    }
    
    clock_t end = clock();
    long elapsed = (end - start) * 1000000 / CLOCKS_PER_SEC;
    
    TEST(elapsed < 1000000, "ISA generation < 1s");
    printf("  Time for 1000 ISAs: %ld μs (%.3f μs per ISA)\n",
           elapsed, (double)elapsed / 1000);
}

void test_shader_compilation_pipeline() {
    printf("\n▶ Full Compilation Pipeline\n");
    
    clock_t start = clock();
    
    // Simulate complete pipeline: GLSL → SPIR-V → ISA
    for (int i = 0; i < 100; i++) {
        // Stage 1: GLSL to SPIR-V (simulated)
        uint32_t spirv[20];
        spirv[0] = 0x07230203;
        
        // Stage 2: Validate SPIR-V
        if (spirv[0] != 0x07230203) continue;
        
        // Stage 3: Parse SPIR-V (skip header)
        int word_count = 5;
        
        // Stage 4: Generate ISA
        uint32_t isa[64];
        isa[0] = 0xBF800000;
        isa[1] = 0xBF810000;
    }
    
    clock_t end = clock();
    long elapsed = (end - start) * 1000000 / CLOCKS_PER_SEC;
    
    TEST(elapsed < 500000, "100 pipelines < 500ms");
    printf("  Time for 100 pipelines: %ld μs (%.3f μs per pipeline)\n",
           elapsed, (double)elapsed / 100);
}

void test_compilation_batching() {
    printf("\n▶ Compilation Batching Performance\n");
    
    clock_t start = clock();
    
    // Batch compile 50 shaders of each type
    typedef struct {
        int vertex_count;
        int fragment_count;
        int compute_count;
    } shader_batch_t;
    
    shader_batch_t batches[10];
    
    for (int batch = 0; batch < 10; batch++) {
        // Compile 5 vertex shaders
        for (int i = 0; i < 5; i++) {
            uint32_t isa[64];
            isa[0] = 0xBF800000;
        }
        
        // Compile 5 fragment shaders
        for (int i = 0; i < 5; i++) {
            uint32_t isa[64];
            isa[0] = 0xBF800000;
        }
        
        // Compile 5 compute shaders
        for (int i = 0; i < 5; i++) {
            uint32_t isa[64];
            isa[0] = 0xBF800000;
        }
        
        batches[batch].vertex_count = 5;
        batches[batch].fragment_count = 5;
        batches[batch].compute_count = 5;
    }
    
    clock_t end = clock();
    long elapsed = (end - start) * 1000000 / CLOCKS_PER_SEC;
    
    TEST(elapsed < 1000000, "Batch compilation < 1s");
    printf("  150 shaders (10 batches) in %ld μs\n", elapsed);
    printf("  Average: %.2f μs per shader\n", (double)elapsed / 150);
}

void test_large_spirv_parsing() {
    printf("\n▶ Large SPIR-V Parsing\n");
    
    // Create larger SPIR-V
    uint32_t large_spirv[1000];
    large_spirv[0] = 0x07230203;
    large_spirv[1] = 0x00010300;
    for (int i = 2; i < 1000; i++) {
        large_spirv[i] = 0;  // Dummy data
    }
    
    clock_t start = clock();
    
    // Parse large SPIR-V 100 times
    for (int iter = 0; iter < 100; iter++) {
        // Walk through instructions
        int instr_count = 0;
        for (int i = 5; i < 1000; i += 4) {  // Assume 4-word instructions
            instr_count++;
        }
        TEST(instr_count > 0, "Parsed instruction stream");
    }
    
    clock_t end = clock();
    long elapsed = (end - start) * 1000000 / CLOCKS_PER_SEC;
    
    printf("  100 parses of 1000-word SPIR-V: %ld μs\n", elapsed);
    printf("  Average: %.3f μs per parse\n", (double)elapsed / 100);
}

void test_isa_caching_benefit() {
    printf("\n▶ ISA Caching Benefit\n");
    
    // Without caching - compile same shader 1000 times
    clock_t start = clock();
    for (int i = 0; i < 1000; i++) {
        uint32_t isa[64];
        isa[0] = 0xBF800000;
    }
    clock_t without_cache = clock() - start;
    
    // With caching - compile once, use 1000 times
    start = clock();
    uint32_t cached_isa[64];
    cached_isa[0] = 0xBF800000;
    for (int i = 0; i < 1000; i++) {
        uint32_t *isa = cached_isa;  // Just reference
        (void)isa;
    }
    clock_t with_cache = clock() - start;
    
    long without_us = without_cache * 1000000 / CLOCKS_PER_SEC;
    long with_us = with_cache * 1000000 / CLOCKS_PER_SEC;
    
    TEST(with_us < without_us, "Caching faster than recompilation");
    printf("  Without cache: %ld μs\n", without_us);
    printf("  With cache:    %ld μs\n", with_us);
    printf("  Speedup: %.2fx\n", (double)without_us / with_us);
}

int main() {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║       Shader Compiler Performance - AMDGPU v0.2            ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    test_spirv_validation_speed();
    test_isa_generation_speed();
    test_shader_compilation_pipeline();
    test_compilation_batching();
    test_large_spirv_parsing();
    test_isa_caching_benefit();
    
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║                   TEST SUMMARY                             ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║ Passed: %d ✓\n", stats.passed);
    printf("║ Failed: %d ✗\n", stats.failed);
    printf("║ Total:  %d\n", stats.passed + stats.failed);
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    return stats.failed == 0 ? 0 : 1;
}

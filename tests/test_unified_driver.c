#include "../src/amd/amd_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define TEST_PASS(name) printf("✓ %s\n", name)
#define TEST_FAIL(name, msg) printf("✗ %s: %s\n", name, msg)

/* Test counters */
static int tests_passed = 0;
static int tests_failed = 0;

/* Test 1: Device Detection */
void test_device_detection(void)
{
    const char *test_name = "Device Detection";
    amd_gpu_device_info_t *gpu = NULL;
    
    /* Test Lucienne (your local hardware) */
    gpu = amd_device_lookup(0x164c);
    assert(gpu != NULL);
    assert(gpu->device_id == 0x164c);
    assert(gpu->generation == AMD_RDNA2);
    assert(strcmp(gpu->codename, "Lucienne") == 0);
    printf("  ✓ Lucienne (RDNA2) detected\n");
    
    /* Test Warrior (your Haiku hardware) */
    gpu = amd_device_lookup(0x9806);
    assert(gpu != NULL);
    assert(gpu->device_id == 0x9806);
    assert(gpu->generation == AMD_VLIW);
    assert(strcmp(gpu->codename, "Wrestler") == 0);
    printf("  ✓ Wrestler (VLIW) detected\n");
    
    /* Test unknown device */
    gpu = amd_device_lookup(0xFFFF);
    assert(gpu == NULL);
    printf("  ✓ Unknown device returns NULL\n");
    
    TEST_PASS(test_name);
    tests_passed++;
}

/* Test 2: Backend Selection */
void test_backend_selection(void)
{
    const char *test_name = "Backend Selection";
    amd_gpu_device_info_t *gpu = NULL;
    amd_backend_type_t backend;
    
    /* RDNA2 should prefer RADV */
    gpu = amd_device_lookup(0x164c);
    backend = amd_select_backend(gpu);
    assert(backend == AMD_BACKEND_RADV);
    printf("  ✓ Lucienne (RDNA2) → RADV\n");
    
    /* VLIW should use Mesa */
    gpu = amd_device_lookup(0x9806);
    backend = amd_select_backend(gpu);
    assert(backend == AMD_BACKEND_MESA);
    printf("  ✓ Wrestler (VLIW) → Mesa\n");
    
    /* NULL device should return SOFTWARE */
    backend = amd_select_backend(NULL);
    assert(backend == AMD_BACKEND_SOFTWARE);
    printf("  ✓ NULL device → Software\n");
    
    TEST_PASS(test_name);
    tests_passed++;
}

/* Test 3: Handler Assignment */
void test_handler_assignment(void)
{
    const char *test_name = "Handler Assignment";
    amd_gpu_handler_t *handler = NULL;
    
    /* VLIW handler */
    handler = amd_get_handler(AMD_VLIW);
    assert(handler != NULL);
    assert(handler->generation == AMD_VLIW);
    assert(handler->hw_init != NULL);
    printf("  ✓ VLIW handler assigned\n");
    
    /* GCN handler */
    handler = amd_get_handler(AMD_GCN2);
    assert(handler != NULL);
    assert(handler->hw_init != NULL);
    printf("  ✓ GCN handler assigned\n");
    
    /* RDNA handler */
    handler = amd_get_handler(AMD_RDNA2);
    assert(handler != NULL);
    assert(handler->hw_init != NULL);
    printf("  ✓ RDNA handler assigned\n");

    TEST_PASS(test_name);
    tests_passed++;
}

/* Test 4: Handler IP Block Integration */
void test_handler_ip_blocks(void)
{
    const char *test_name = "Handler IP Block Integration";
    amd_gpu_handler_t *handler = NULL;

    /* Test GCN handler has IP blocks */
    handler = amd_get_handler(AMD_GCN2);
    assert(handler != NULL);
    assert(handler->ip_blocks.gmc != NULL);
    assert(handler->ip_blocks.gfx != NULL);
    assert(handler->ip_blocks.display != NULL);
    printf("  ✓ GCN handler has IP blocks assigned\n");

    /* Test VLIW handler has IP blocks */
    handler = amd_get_handler(AMD_VLIW);
    assert(handler != NULL);
    assert(handler->ip_blocks.gmc != NULL);
    assert(handler->ip_blocks.gfx != NULL);
    printf("  ✓ VLIW handler has IP blocks assigned\n");

    /* Test RDNA handler has IP blocks */
    handler = amd_get_handler(AMD_RDNA2);
    assert(handler != NULL);
    assert(handler->ip_blocks.gmc != NULL);
    assert(handler->ip_blocks.gfx != NULL);
    printf("  ✓ RDNA handler has IP blocks assigned\n");

    TEST_PASS(test_name);
    tests_passed++;
}

/* Test 4: Device Probe */
void test_device_probe(void)
{
    const char *test_name = "Device Probe";
    amd_device_t *dev = NULL;
    int ret = 0;
    
    /* Probe Lucienne */
    ret = amd_device_probe(0x164c, &dev);
    assert(ret == 0);
    assert(dev != NULL);
    assert(dev->gpu_info.device_id == 0x164c);
    assert(dev->handler != NULL);
    amd_device_free(dev);
    printf("  ✓ Lucienne probe successful\n");
    
    /* Probe Wrestler */
    dev = NULL;
    ret = amd_device_probe(0x9806, &dev);
    assert(ret == 0);
    assert(dev != NULL);
    assert(dev->gpu_info.device_id == 0x9806);
    amd_device_free(dev);
    printf("  ✓ Wrestler probe successful\n");
    
    /* Probe unknown device */
    dev = NULL;
    ret = amd_device_probe(0xFFFF, &dev);
    assert(ret != 0);
    assert(dev == NULL);
    printf("  ✓ Unknown device probe fails correctly\n");
    
    TEST_PASS(test_name);
    tests_passed++;
}

/* Test 5: Device Initialization (Safe) */
void test_device_init(void)
{
    const char *test_name = "Device Initialization";
    amd_device_t *dev = NULL;
    int ret = 0;
    
    /* Init Lucienne */
    printf("  Testing Lucienne (RDNA2)...\n");
    ret = amd_device_probe(0x164c, &dev);
    assert(ret == 0);
    
    ret = amd_device_init(dev);
    assert(ret == 0);
    assert(dev->initialized == true);
    printf("    ✓ Initialization successful\n");
    
    /* Allocate VRAM */
    uint64_t gpu_addr = 0;
    ret = dev->handler->allocate_vram(dev, 1024 * 1024, &gpu_addr);
    assert(ret == 0);
    assert(gpu_addr != 0);
    printf("    ✓ VRAM allocation successful\n");
    
    /* Cleanup */
    ret = amd_device_fini(dev);
    assert(ret == 0);
    assert(dev->initialized == false);
    printf("    ✓ Finalization successful\n");
    
    amd_device_free(dev);
    
    /* Init Wrestler */
    printf("  Testing Wrestler (VLIW)...\n");
    dev = NULL;
    ret = amd_device_probe(0x9806, &dev);
    assert(ret == 0);
    
    ret = amd_device_init(dev);
    assert(ret == 0);
    assert(dev->initialized == true);
    printf("    ✓ Initialization successful\n");
    
    ret = amd_device_fini(dev);
    assert(ret == 0);
    amd_device_free(dev);
    printf("    ✓ Finalization successful\n");
    
    TEST_PASS(test_name);
    tests_passed++;
}

/* Test 6: Multi-GPU Support */
void test_multi_gpu(void)
{
    const char *test_name = "Multi-GPU Support";
    amd_device_t *dev1 = NULL, *dev2 = NULL;
    int ret = 0;
    
    /* Init two different GPUs */
    ret = amd_device_probe(0x164c, &dev1);
    assert(ret == 0);
    ret = amd_device_init(dev1);
    assert(ret == 0);
    printf("  ✓ Lucienne initialized\n");
    
    ret = amd_device_probe(0x9806, &dev2);
    assert(ret == 0);
    ret = amd_device_init(dev2);
    assert(ret == 0);
    printf("  ✓ Wrestler initialized\n");
    
    /* Both should be independent */
    assert(dev1->gpu_info.device_id != dev2->gpu_info.device_id);
    assert(dev1->gpu_info.generation != dev2->gpu_info.generation);
    printf("  ✓ Devices are independent\n");
    
    /* Cleanup both */
    amd_device_fini(dev1);
    amd_device_free(dev1);
    amd_device_fini(dev2);
    amd_device_free(dev2);
    printf("  ✓ Both devices finalized correctly\n");
    
    TEST_PASS(test_name);
    tests_passed++;
}

/* Test 7: Device Info Printing (Debug) */
void test_device_info_print(void)
{
    const char *test_name = "Device Info Printing";
    amd_gpu_device_info_t *gpu = NULL;
    
    printf("  Lucienne GPU Info:\n");
    gpu = amd_device_lookup(0x164c);
    amd_device_print_info(gpu);
    printf("\n  Wrestler GPU Info:\n");
    gpu = amd_device_lookup(0x9806);
    amd_device_print_info(gpu);
    
    TEST_PASS(test_name);
    tests_passed++;
}

int main(void)
{
    printf("=== AMD Unified Driver Test Suite ===\n\n");
    
    test_device_detection();
    printf("\n");

    test_handler_assignment();
    printf("\n");

    test_handler_ip_blocks();
    printf("\n");

    test_device_probe();
    printf("\n");
    test_handler_assignment();
    printf("\n");
    test_device_probe();
    printf("\n");
    test_device_init();
    printf("\n");
    test_multi_gpu();
    printf("\n");
    test_device_info_print();
    
    printf("\n=== Test Summary ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n\n", tests_passed + tests_failed);
    
    return (tests_failed == 0) ? 0 : 1;
}

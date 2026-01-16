/*
 * PCI Detection Test Program
 * Tests real hardware enumeration using POSIX APIs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Include our OS primitives interface */
#include "kernel-amd/os-primitives/os_primitives.h"

int main(int argc, char *argv[]) {
    printf("=== AMD GPU PCI Detection Test ===\n\n");
    
    /* Test 1: Generic AMD Radeon */
    printf("Test 1: Looking for AMD Radeon (1002:xxxx)...\n");
    void *amd_gpu = NULL;
    int ret = os_prim_pci_find_device(0x1002, 0x0000, &amd_gpu);
    if (ret == 0 && amd_gpu) {
        printf("  ✓ Found AMD GPU\n");
        uint32_t vendor_device = 0;
        os_prim_pci_read_config(amd_gpu, 0, &vendor_device);
        printf("  Vendor:Device = 0x%08x\n", vendor_device);
    } else {
        printf("  ✗ No AMD GPU found (normal if not present)\n");
    }
    
    /* Test 2: Intel iGPU */
    printf("\nTest 2: Looking for Intel iGPU (8086:xxxx)...\n");
    void *intel_gpu = NULL;
    ret = os_prim_pci_find_device(0x8086, 0x0000, &intel_gpu);
    if (ret == 0 && intel_gpu) {
        printf("  ✓ Found Intel GPU\n");
        uint32_t vendor_device = 0;
        os_prim_pci_read_config(intel_gpu, 0, &vendor_device);
        printf("  Vendor:Device = 0x%08x\n", vendor_device);
    } else {
        printf("  ✗ No Intel GPU found (normal if not present)\n");
    }
    
    /* Test 3: Try to map a resource if found */
    if (amd_gpu) {
        printf("\nTest 3: Mapping PCI resource...\n");
        void *mem = os_prim_pci_map_resource(amd_gpu, 0);
        if (mem) {
            printf("  ✓ Mapped BAR0 to %p\n", mem);
            
            /* Try to read first DWORD */
            uint32_t value = os_prim_read32((uintptr_t)mem);
            printf("  BAR0[0] = 0x%08x\n", value);
            
            os_prim_pci_unmap_resource(mem);
            printf("  ✓ Unmapped\n");
        } else {
            printf("  ✗ Could not map BAR0\n");
        }
    }
    
    printf("\n=== Test Complete ===\n");
    return 0;
}

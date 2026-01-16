// Vkinfo-like program using AMD driver HAL
// Shows GPU info in Vulkan-style format

#include <stdio.h>
#include "kernel-amd/os-interface/os_interface.h"
#include "src/amd/hal.h"

extern int amdgpu_device_init(struct OBJGPU* adev);
extern void amdgpu_device_fini(struct OBJGPU* adev);

int main() {
    printf("=== Vkinfo AMD (simulado via driver) ===\n");

    // Init driver
    struct OBJGPU dev;
    dev.mmio_base = (void*)0x10000000;
    amdgpu_device_init(&dev);

    // Get GPU info via HAL
    struct amdgpu_gpu_info info;
    if (amdgpu_gpu_get_info_hal(&dev, &info) == 0) {
        printf("GPU 0: %s (RADV)\n", info.gpu_name);
        printf("    driver: radv 1.0.0\n");
        printf("    deviceType: discreteGpu\n");
        printf("    vendorID: 0x1002\n");
        printf("    deviceID: 0x7310\n");
        printf("    memoryHeapCount: 1\n");
        printf("        memoryHeaps[0]:\n");
        printf("            size: %u MB\n", info.vram_size_mb);
        printf("            flags: 0x1 (DEVICE_LOCAL)\n");
        printf("    queueFamilyCount: 1\n");
        printf("        queueFamilies[0]:\n");
        printf("            queueCount: 1\n");
        printf("            queueFlags: 0x1f (GRAPHICS | COMPUTE | TRANSFER | SPARSE)\n");
        printf("    apiVersion: 1.3.0\n");
        printf("    driverVersion: %u\n", info.gpu_clock_mhz); // Fake
    } else {
        printf("Error obteniendo GPU info\n");
    }

    // Cleanup
    amdgpu_device_fini(&dev);
    return 0;
}
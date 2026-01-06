// Abstracted AMD GPU Device Driver
// OS-agnostic hardware layer inspired by NVIDIA HAL

#include "../../kernel-amd/os-interface/os_interface.h"
#include "hal.h"

// Global OS interface
static struct os_interface* os_if;

// Init function
int amdgpu_device_init(struct OBJGPU* adev) {
    os_if = os_get_interface();

    // Detect ASIC (stub)
    adev->asic_type = 0x1000; // CHIP_NAVI10 stub
    // Set HAL ops based on ASIC
    if (adev->asic_type == 0x1000) {
        adev->hal_ops = &navi10_hal_ops;
    }

    // Call HAL init
    return amdgpu_device_init_hal(adev);
}

// Cleanup
void amdgpu_device_fini(struct OBJGPU* adev) {
    if (adev->hal_ops && adev->hal_ops->device_fini) {
        adev->hal_ops->device_fini(adev);
    }
}

// Register read/write macros
#define AMD_REG_RD32(adev, reg) (*(volatile uint32_t*)((uint8_t*)adev->mmio_base + reg))
#define AMD_REG_WR32(adev, reg, val) (*(volatile uint32_t*)((uint8_t*)adev->mmio_base + reg) = val)
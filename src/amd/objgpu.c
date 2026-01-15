// Abstracted AMD GPU Device Driver
// OS-agnostic hardware layer inspired by NVIDIA HAL
// Developed by: Haiku Imposible Team

#include "../../kernel-amd/os-interface/os_interface.h"
#include "hal.h"

// Global OS interface
static struct os_interface *os_if;

// Init function
int amdgpu_device_init(struct OBJGPU *adev) {
  os_if = os_get_interface();

  // Detect ASIC (stub)
  adev->asic_type = 0x1000; // CHIP_NAVI10 stub
  adev->family = 10;        // NV family

  // Call HAL init (which handles IP Block registration)
  return amdgpu_device_init_hal(adev);
}

// Cleanup
void amdgpu_device_fini(struct OBJGPU *adev) { amdgpu_device_fini_hal(adev); }

// Register read/write macros
#define AMD_REG_RD32(adev, reg)                                                \
  (*(volatile uint32_t *)((uint8_t *)adev->mmio_base + reg))
#define AMD_REG_WR32(adev, reg, val)                                           \
  (*(volatile uint32_t *)((uint8_t *)adev->mmio_base + reg) = val)

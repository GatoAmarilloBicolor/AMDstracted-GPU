#include "../interface/driver_interface.h"
#include "../../os/os_interface.h"
#include "amdgpu_pci_ids.h"
#include "../../core/hal/hal.h"
#include <string.h>

// AMD GPU driver interface implementation
static int amdgpu_probe(struct os_pci_device *dev) {
    // Check if device is AMD GPU
    uint16_t vendor, device;
    os_get_interface()->prim_pci_get_ids(dev->handle, &vendor, &device);

    if (vendor != 0x1002) { // AMD vendor ID
        return -1;
    }

    // Check if device ID is in AMD PCI table
    for (int i = 0; amd_pci_table[i].device_id != 0; i++) {
        if (amd_pci_table[i].device_id == device) {
            return 0; // Supported
        }
    }

    return -1; // Not supported
}

static int amdgpu_init(struct OBJGPU *adev) {
    // Initialize AMD GPU device
    return amdgpu_device_init_hal(adev);
}

static void amdgpu_fini(struct OBJGPU *adev) {
    // Finalize AMD GPU device
    amdgpu_device_fini_hal(adev);
}

static int amdgpu_suspend(struct OBJGPU *adev) {
    // Suspend operations (stub)
    return 0;
}

static int amdgpu_resume(struct OBJGPU *adev) {
    // Resume operations (stub)
    return 0;
}

// AMD driver interface structure
struct driver_interface amdgpu_driver = {
    .name = "amdgpu",
    .vendor = "AMD",
    .probe = amdgpu_probe,
    .init = amdgpu_init,
    .fini = amdgpu_fini,
    .suspend = amdgpu_suspend,
    .resume = amdgpu_resume,
    .version_major = 1,
    .version_minor = 0
};

// Registration function (call this to register the driver)
int amdgpu_driver_register(void) {
    return driver_register(&amdgpu_driver);
}
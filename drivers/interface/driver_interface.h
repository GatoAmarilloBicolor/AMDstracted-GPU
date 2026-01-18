#ifndef DRIVER_INTERFACE_H
#define DRIVER_INTERFACE_H

#include <stdint.h>

// Forward declarations
struct os_pci_device;
struct OBJGPU;

// Driver interface structure
struct driver_interface {
    const char *name;  // Driver name (e.g., "amdgpu", "nvidia")
    const char *vendor; // Vendor string (e.g., "AMD", "NVIDIA")

    // Probe: Check if driver supports this device
    int (*probe)(struct os_pci_device *dev);

    // Initialize device
    int (*init)(struct OBJGPU *adev);

    // Finalize device
    void (*fini)(struct OBJGPU *adev);

    // Additional operations (optional)
    int (*suspend)(struct OBJGPU *adev);
    int (*resume)(struct OBJGPU *adev);

    // Version info
    uint32_t version_major;
    uint32_t version_minor;
};

// Driver registration
int driver_register(struct driver_interface *driver);
void driver_unregister(struct driver_interface *driver);

// Driver discovery and loading
int driver_probe_device(struct os_pci_device *dev, struct driver_interface **driver_out);
int driver_init_device(struct driver_interface *driver, struct OBJGPU *adev);

// Get list of registered drivers
struct driver_interface **driver_get_list(int *count);

#endif // DRIVER_INTERFACE_H
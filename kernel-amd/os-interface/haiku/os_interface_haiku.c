#include "../os_interface.h"
#include "../../../kernel-amd/os-primitives/os_primitives.h"
// Stubs for Haiku API (when not available)
#ifndef B_DEVICE_OP_CODES_END
#define B_DEVICE_OP_CODES_END 0x1000
#endif
#ifndef B_OS_NAME_LENGTH
#define B_OS_NAME_LENGTH 64
#endif
typedef unsigned int uint32; // Stub

// Implementations using primitives for Haiku (inspired by NVIDIA-Haiku)

// AMD device names (adapted from NVIDIA)
#define AMDGPU_CONTROL_DEVICE_NAME "amdgpuctl"
#define AMDGPU_DEVICE_NAME "graphics/amdgpu"

enum {
    AMD_HAIKU_BASE = B_DEVICE_OP_CODES_END + 1, // Like NV_HAIKU_BASE
};

enum {
    AMD_HAIKU_GET_COOKIE = 0, // kernel only
    AMD_HAIKU_MAP,
};

typedef struct {
    char name[B_OS_NAME_LENGTH]; // Like NVIDIA: use Haiku API
    void *address;
    uint32 addressSpec;
    uint32 protection;
} amd_haiku_map_params;

int os_pci_find_device(uint16_t vendor, uint16_t device, os_pci_device* dev) {
    return os_prim_pci_find_device(vendor, device, &dev->handle);
}

uint32_t os_pci_read_config(os_pci_device* dev, int offset) {
    uint32_t val;
    os_prim_pci_read_config(dev->handle, offset, &val);
    return val;
}

void os_pci_write_config(os_pci_device* dev, int offset, uint32_t val) {
    os_prim_pci_write_config(dev->handle, offset, val);
}

void* os_pci_map_resource(os_pci_device* dev, int bar, size_t* size) {
    *size = 0;
    return os_prim_pci_map_resource(dev->handle, bar);
}

void os_pci_unmap_resource(void* addr, size_t size) {
    os_prim_pci_unmap_resource(addr);
}

int os_register_interrupt(int irq, os_prim_interrupt_handler handler, void* data) {
    return os_prim_register_interrupt(irq, handler, data);
}

void os_unregister_interrupt(int irq) {
    os_prim_unregister_interrupt(irq);
}

int os_display_init(void) {
    return os_prim_display_init();
}

void os_display_put_pixel(int x, int y, uint32_t color) {
    os_prim_display_put_pixel(x, y, color);
}

struct os_interface haiku_os_interface = {
    // Empty
};

struct os_interface* os_get_interface(void) {
    return &haiku_os_interface;
}
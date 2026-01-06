#include "../os_interface.h"
#include "../../kernel-amd/os-primitives/os_primitives.h"

// Implementations using primitives

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
    // Stub size
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

struct os_interface netbsd_os_interface = {
    // Empty for now, functions are global
};

struct os_interface* os_get_interface(void) {
    return &netbsd_os_interface;
}
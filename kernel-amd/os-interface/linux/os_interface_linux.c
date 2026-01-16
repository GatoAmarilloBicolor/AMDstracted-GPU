#include "../os_interface.h"
#include "../../os-primitives/os_primitives.h"

/*
 * 🌀 HIT Edition: Linux OS Interface
 * Provides OS-specific interface layer on top of primitives
 */

// PCI Interface Implementation
int os_pci_find_device(uint16_t vendor, uint16_t device, os_pci_device *dev) {
    return os_prim_pci_find_device(vendor, device, &dev->handle);
}

uint32_t os_pci_read_config(os_pci_device *dev, int offset) {
    uint32_t val;
    os_prim_pci_read_config(dev->handle, offset, &val);
    return val;
}

void os_pci_write_config(os_pci_device *dev, int offset, uint32_t val) {
    os_prim_pci_write_config(dev->handle, offset, val);
}

void *os_pci_map_resource(os_pci_device *dev, int bar, size_t *size) {
    *size = 4096;  // Assume 4KB for now
    return os_prim_pci_map_resource(dev->handle, bar);
}

void os_pci_unmap_resource(void *addr, size_t size) {
    os_prim_pci_unmap_resource(addr);
}

// Interrupt Interface Implementation
int os_register_interrupt(int irq, os_prim_interrupt_handler handler, void *data) {
    return os_prim_register_interrupt(irq, handler, data);
}

void os_unregister_interrupt(int irq) {
    os_prim_unregister_interrupt(irq);
}

// Display Interface Implementation
int os_display_init(void) {
    return os_prim_display_init();
}

void os_display_put_pixel(int x, int y, uint32_t color) {
    os_prim_display_put_pixel(x, y, color);
}

// The "Toolbox" (Interface) structure for Linux
struct os_interface linux_os_interface = {
    .mem = {0},      // Placeholder
    .intr = {0},     // Placeholder
    .disp = {0},     // Placeholder
    .pci = {0},      // Placeholder
};

// Giving the driver access to the Linux toolbox
struct os_interface *os_get_interface(void) {
    return &linux_os_interface;
}

int os_interface_init(void) {
    return 0;
}

void os_interface_fini(void) {
}

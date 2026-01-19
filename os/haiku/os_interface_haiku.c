#include "../os_interface.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <drivers/PCI.h>
#include <kernel/OS.h>
#include <kernel/image.h>

// Forward declarations of primitives
void *os_prim_alloc(size_t size);
void os_prim_free(void *ptr);
void os_prim_log(const char *fmt, ...);
int os_prim_pci_find_device(uint16_t vendor, uint16_t device, void **handle);
void os_prim_pci_get_ids(void *pci_handle, uint16_t *vendor, uint16_t *device);
void *os_prim_pci_map_resource(void *pci_handle, int bar);
void os_prim_write32(uintptr_t addr, uint32_t val);
uint32_t os_prim_read32(uintptr_t addr);
void os_prim_delay_us(unsigned int us);

// Haiku implementations
// Note: Full PCI access from userland requires kernel driver context
// These stubs allow compilation but return errors for direct GPU access

int haiku_pci_find_device(uint16_t vendor, uint16_t device, os_pci_device *dev) {
    // Stub: Haiku PCI access not available in userland
    (void)vendor;
    (void)device;
    dev->handle = NULL;
    return -1;
}

uint32_t haiku_pci_read_config(os_pci_device *dev, int offset) {
    if (!dev || !dev->handle) return 0;
    pci_info *info = (pci_info *)dev->handle;
    // Read PCI config space
    // Haiku has pci_read_config etc.
    // For now, stub
    return 0;
}

void haiku_pci_write_config(os_pci_device *dev, int offset, uint32_t val) {
    // Stub
}

void *haiku_pci_map_resource(os_pci_device *dev, int bar, size_t *size) {
    // Stub: Physical memory mapping requires kernel driver context
    (void)dev;
    (void)bar;
    *size = 0;
    return NULL;
}

void haiku_pci_unmap_resource(void *addr, size_t size) {
    if (addr) {
        // delete_area, but need area_id, complicated
    }
}

int haiku_register_interrupt(int irq, os_interrupt_handler handler, void *data) {
    // Haiku interrupt handling
    return -1; // Stub
}

void haiku_unregister_interrupt(int irq) {
    // Stub
}

int haiku_display_init(void) {
    return -1; // Stub
}

void haiku_display_put_pixel(int x, int y, uint32_t color) {
    // Stub
}

// Haiku OS interface struct
struct os_interface haiku_os_interface = {
    .pci_find_device = haiku_pci_find_device,
    .pci_read_config = haiku_pci_read_config,
    .pci_write_config = haiku_pci_write_config,
    .pci_map_resource = haiku_pci_map_resource,
    .pci_unmap_resource = haiku_pci_unmap_resource,
    .register_interrupt = haiku_register_interrupt,
    .unregister_interrupt = haiku_unregister_interrupt,
    .display_init = haiku_display_init,
    .display_put_pixel = haiku_display_put_pixel,
    .alloc = os_prim_alloc,
    .free = os_prim_free,
    .log = os_prim_log,
    .prim_pci_find_device = os_prim_pci_find_device,
    .prim_pci_get_ids = os_prim_pci_get_ids,
    .prim_pci_map_resource = os_prim_pci_map_resource,
    .write32 = os_prim_write32,
    .read32 = os_prim_read32,
    .delay_us = os_prim_delay_us
};

struct os_interface *os_get_interface(void) {
    return &haiku_os_interface;
}
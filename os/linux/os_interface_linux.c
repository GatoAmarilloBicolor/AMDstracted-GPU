#include "../os_interface.h"
#include <stddef.h>

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

// Linux implementations (stubs for now)
int linux_pci_find_device(uint16_t vendor, uint16_t device, os_pci_device *dev) {
    // Stub
    dev->handle = NULL;
    return -1;
}

uint32_t linux_pci_read_config(os_pci_device *dev, int offset) {
    return 0;
}

void linux_pci_write_config(os_pci_device *dev, int offset, uint32_t val) {
    // Stub
}

void *linux_pci_map_resource(os_pci_device *dev, int bar, size_t *size) {
    *size = 0;
    return NULL;
}

void linux_pci_unmap_resource(void *addr, size_t size) {
    // Stub
}

int linux_register_interrupt(int irq, os_interrupt_handler handler, void *data) {
    return -1;
}

void linux_unregister_interrupt(int irq) {
    // Stub
}

int linux_display_init(void) {
    return -1;
}

void linux_display_put_pixel(int x, int y, uint32_t color) {
    // Stub
}

// Linux OS interface struct
struct os_interface linux_os_interface = {
    .pci_find_device = linux_pci_find_device,
    .pci_read_config = linux_pci_read_config,
    .pci_write_config = linux_pci_write_config,
    .pci_map_resource = linux_pci_map_resource,
    .pci_unmap_resource = linux_pci_unmap_resource,
    .register_interrupt = linux_register_interrupt,
    .unregister_interrupt = linux_unregister_interrupt,
    .display_init = linux_display_init,
    .display_put_pixel = linux_display_put_pixel,
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
    return &linux_os_interface;
}
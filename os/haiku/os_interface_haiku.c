#include "../os_interface.h"
#include <stddef.h>

// Forward declarations
void *os_prim_alloc(size_t size);
void os_prim_free(void *ptr);
void os_prim_log(const char *fmt, ...);
int os_prim_pci_find_device(uint16_t vendor, uint16_t device, void **handle);
void os_prim_pci_get_ids(void *pci_handle, uint16_t *vendor, uint16_t *device);
void *os_prim_pci_map_resource(void *pci_handle, int bar);
void os_prim_write32(uintptr_t addr, uint32_t val);
uint32_t os_prim_read32(uintptr_t addr);
void os_prim_delay_us(unsigned int us);

// Haiku implementations (stubs)
int haiku_pci_find_device(uint16_t vendor, uint16_t device, os_pci_device *dev) {
    (void)vendor; (void)device;
    // Stub: In Haiku, use PCI bus scanning
    dev->handle = NULL;
    return -1;
}

uint32_t haiku_pci_read_config(os_pci_device *dev, int offset) {
    (void)dev; (void)offset;
    return 0;
}

void haiku_pci_write_config(os_pci_device *dev, int offset, uint32_t val) {
    (void)dev; (void)offset; (void)val;
    // Stub
}

void *haiku_pci_map_resource(os_pci_device *dev, int bar, size_t *size) {
    (void)dev; (void)bar;
    *size = 0;
    return NULL;
}

void haiku_pci_unmap_resource(void *addr, size_t size) {
    (void)addr; (void)size;
    // Stub
}

int haiku_register_interrupt(int irq, os_interrupt_handler handler, void *data) {
    (void)irq; (void)handler; (void)data;
    return -1;
}

void haiku_unregister_interrupt(int irq) {
    (void)irq;
    // Stub
}

int haiku_display_init(void) {
    return -1;
}

void haiku_display_put_pixel(int x, int y, uint32_t color) {
    (void)x; (void)y; (void)color;
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
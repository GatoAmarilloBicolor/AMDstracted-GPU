#include "../os_interface.h"
#include <kernel/OS.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/* Forward declarations from generic stub */
extern int os_pci_find_device(uint16_t vendor, uint16_t device, void **handle);
extern int os_prim_pci_find_device(uint16_t vendor, uint16_t device, void **handle);
extern void os_prim_pci_get_ids(void *pci_handle, uint16_t *vendor, uint16_t *device);
extern void *os_prim_pci_map_resource(void *pci_handle, int bar);
extern uint32_t os_pci_read_config(void *pci_handle, int offset);
extern void os_pci_write_config(void *pci_handle, int offset, uint32_t val);
extern void os_pci_unmap_resource(void *addr, size_t size);
extern int os_register_interrupt(int irq, void (*handler)(void *), void *data);
extern void os_unregister_interrupt(int irq);
extern int os_display_init(void);
extern void os_display_put_pixel(int x, int y, uint32_t color);
extern void *os_prim_alloc(size_t size);
extern void os_prim_free(void *ptr);
extern void os_prim_log(const char *fmt, ...);
extern void os_prim_write32(uintptr_t addr, uint32_t val);
extern uint32_t os_prim_read32(uintptr_t addr);
extern void os_prim_delay_us(unsigned int us);

/* Haiku-specific implementations */
void haiku_delay_us(unsigned int us) {
    snooze(us);
}

void haiku_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

/* Wrapper functions to match interface signatures */
int haiku_pci_find_device(uint16_t vendor, uint16_t device, os_pci_device *dev) {
    return os_pci_find_device(vendor, device, &dev->handle);
}

uint32_t haiku_pci_read_config(os_pci_device *dev, int offset) {
    return os_pci_read_config(dev->handle, offset);
}

void haiku_pci_write_config(os_pci_device *dev, int offset, uint32_t val) {
    os_pci_write_config(dev->handle, offset, val);
}

void *haiku_pci_map_resource(os_pci_device *dev, int bar, size_t *size) {
    *size = 4096; // Assume 4KB
    return os_prim_pci_map_resource(dev->handle, bar);
}

/* Haiku OS interface - uses generic stubs + Haiku-specific functions */
struct os_interface haiku_os_interface = {
    .pci_find_device = haiku_pci_find_device,
    .pci_read_config = haiku_pci_read_config,
    .pci_write_config = haiku_pci_write_config,
    .pci_map_resource = haiku_pci_map_resource,
    .pci_unmap_resource = os_pci_unmap_resource,
    .register_interrupt = os_register_interrupt,
    .unregister_interrupt = os_unregister_interrupt,
    .display_init = os_display_init,
    .display_put_pixel = os_display_put_pixel,
    .alloc = os_prim_alloc,
    .free = os_prim_free,
    .log = os_prim_log,
    .prim_pci_find_device = os_prim_pci_find_device,
    .prim_pci_get_ids = os_prim_pci_get_ids,
    .prim_pci_map_resource = os_prim_pci_map_resource,
    .write32 = os_prim_write32,
    .read32 = os_prim_read32,
    .delay_us = haiku_delay_us
};

struct os_interface *os_get_interface(void) {
    return &haiku_os_interface;
}

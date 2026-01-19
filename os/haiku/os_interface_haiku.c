#include "../os_interface.h"
#include <kernel/OS.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/* Forward declarations from generic stub */
extern int pci_stub_find_device(uint16_t vendor, uint16_t device, void **handle);
extern void pci_stub_get_ids(void *pci_handle, uint16_t *vendor, uint16_t *device);
extern void *pci_stub_map_resource(void *pci_handle, int bar);
extern uint32_t pci_stub_read_config(void *pci_handle, int offset);
extern void pci_stub_write_config(void *pci_handle, int offset, uint32_t val);
extern void pci_stub_unmap_resource(void *addr, size_t size);
extern int pci_stub_register_interrupt(int irq, void (*handler)(void *), void *data);
extern void pci_stub_unregister_interrupt(int irq);
extern int pci_stub_display_init(void);
extern void pci_stub_display_put_pixel(int x, int y, uint32_t color);
extern void *pci_stub_alloc(size_t size);
extern void pci_stub_free(void *ptr);
extern void pci_stub_log(const char *fmt, ...);
extern void pci_stub_write32(uintptr_t addr, uint32_t val);
extern uint32_t pci_stub_read32(uintptr_t addr);
extern void pci_stub_delay_us(unsigned int us);

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

/* Haiku OS interface - uses generic stubs + Haiku-specific functions */
struct os_interface haiku_os_interface = {
    .pci_find_device = (void *)pci_stub_find_device,
    .pci_read_config = (void *)pci_stub_read_config,
    .pci_write_config = (void *)pci_stub_write_config,
    .pci_map_resource = (void *)pci_stub_map_resource,
    .pci_unmap_resource = (void *)pci_stub_unmap_resource,
    .register_interrupt = (void *)pci_stub_register_interrupt,
    .unregister_interrupt = (void *)pci_stub_unregister_interrupt,
    .display_init = (void *)pci_stub_display_init,
    .display_put_pixel = (void *)pci_stub_display_put_pixel,
    .alloc = (void *)pci_stub_alloc,
    .free = (void *)pci_stub_free,
    .log = (void *)haiku_log,
    .prim_pci_find_device = (void *)pci_stub_find_device,
    .prim_pci_get_ids = (void *)pci_stub_get_ids,
    .prim_pci_map_resource = (void *)pci_stub_map_resource,
    .write32 = (void *)pci_stub_write32,
    .read32 = (void *)pci_stub_read32,
    .delay_us = (void *)haiku_delay_us
};

struct os_interface *os_get_interface(void) {
    return &haiku_os_interface;
}

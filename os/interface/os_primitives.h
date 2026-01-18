#ifndef OS_PRIMITIVES_H
#define OS_PRIMITIVES_H

#include <stdint.h>
#include <stddef.h>

// Logging
void os_prim_log(const char *fmt, ...);

// Memory management
void *os_prim_alloc(size_t size);
void os_prim_free(void *ptr);

// PCI primitives
int os_prim_pci_find_device(uint16_t vendor, uint16_t device, void **handle);
void os_prim_pci_get_ids(void *pci_handle, uint16_t *vendor, uint16_t *device);
void *os_prim_pci_map_resource(void *pci_handle, int bar);

// MMIO access
void os_prim_write32(uintptr_t addr, uint32_t val);
uint32_t os_prim_read32(uintptr_t addr);

// Delay
void os_prim_delay_us(unsigned int us);

#endif // OS_PRIMITIVES_H
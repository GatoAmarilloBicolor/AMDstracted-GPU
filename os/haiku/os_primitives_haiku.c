#define _DEFAULT_SOURCE
#include "../interface/os_primitives.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>

// Logging
void os_prim_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

// Memory management
void *os_prim_alloc(size_t size) {
    return malloc(size);
}

void os_prim_free(void *ptr) {
    free(ptr);
}

// PCI primitives (stub implementations for Haiku)
int os_prim_pci_find_device(uint16_t vendor, uint16_t device, void **handle) {
    // Stub: In Haiku, use PCI bus
    *handle = (void *)0x1;
    return 0;
}

void os_prim_pci_get_ids(void *pci_handle, uint16_t *vendor, uint16_t *device) {
    *vendor = 0x1002; // AMD
    *device = 0x7310; // Navi10
}

void *os_prim_pci_map_resource(void *pci_handle, int bar) {
    // Stub: return a fake MMIO address
    return (void *)0x10000000; // Fake address
}

// MMIO access (stub)
void os_prim_write32(uintptr_t addr, uint32_t val) {
    // In real implementation, write to MMIO
    // For now, do nothing
}

uint32_t os_prim_read32(uintptr_t addr) {
    // In real implementation, read from MMIO
    return 0;
}

// Delay
void os_prim_delay_us(unsigned int us) {
    usleep(us);
}
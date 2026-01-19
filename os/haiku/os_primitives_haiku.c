#define _DEFAULT_SOURCE
#include "../interface/os_primitives.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <drivers/PCI.h>
#include <kernel/OS.h>

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

// PCI primitives using Haiku PCI bus manager
// Note: Haiku PCI access requires special permissions and kernel support
// For now, we provide stubs that fail gracefully with error codes

int os_prim_pci_find_device(uint16_t vendor, uint16_t device, void **handle) {
    // Stub: Haiku PCI access not available in userland without proper driver
    // This would require using Haiku's device_manager or kernel driver interface
    *handle = NULL;
    return -1;  // PCI device not found (stub)
}

void os_prim_pci_get_ids(void *pci_handle, uint16_t *vendor, uint16_t *device) {
    if (pci_handle) {
        pci_info *info = (pci_info *)pci_handle;
        *vendor = info->vendor_id;
        *device = info->device_id;
    }
}

void *os_prim_pci_map_resource(void *pci_handle, int bar) {
    // Stub: Physical memory mapping requires kernel driver context
    // Cannot be done safely from userland without proper driver framework
    (void)pci_handle;
    (void)bar;
    return NULL;
}

// MMIO access
void os_prim_write32(uintptr_t addr, uint32_t val) {
    if (addr) {
        *(volatile uint32_t *)addr = val;
    }
}

uint32_t os_prim_read32(uintptr_t addr) {
    if (addr) {
        return *(volatile uint32_t *)addr;
    }
    return 0;
}

// Delay
void os_prim_delay_us(unsigned int us) {
    snooze(us);
}
#define _DEFAULT_SOURCE
#include "../interface/os_primitives.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <drivers/PCI.h>
#include <drivers/KernelExport.h>
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
int os_prim_pci_find_device(uint16_t vendor, uint16_t device, void **handle) {
    pci_info info;
    int index = 0;
    while (get_nth_pci_info(index, &info) == B_OK) {
        if (info.vendor_id == vendor && info.device_id == device) {
            *handle = malloc(sizeof(pci_info));
            if (!*handle) return -1;
            memcpy(*handle, &info, sizeof(pci_info));
            return 0;
        }
        index++;
    }
    return -1;
}

void os_prim_pci_get_ids(void *pci_handle, uint16_t *vendor, uint16_t *device) {
    if (pci_handle) {
        pci_info *info = (pci_info *)pci_handle;
        *vendor = info->vendor_id;
        *device = info->device_id;
    }
}

void *os_prim_pci_map_resource(void *pci_handle, int bar) {
    if (!pci_handle || bar < 0 || bar >= 6) return NULL;
    
    pci_info *info = (pci_info *)pci_handle;
    uint64_t addr = info->u.h0.base_registers[bar];
    uint64_t size = info->u.h0.base_register_sizes[bar];
    
    if (size == 0) return NULL;
    
    area_id area;
    void *virt_addr;
    
    area = map_physical_memory("AMD GPU MMIO", addr, size, B_ANY_KERNEL_ADDRESS,
                              B_READ_AREA | B_WRITE_AREA, &virt_addr);
    
    if (area < 0) return NULL;
    
    return virt_addr;
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
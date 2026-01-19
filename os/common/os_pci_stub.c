/* 
 * Generic PCI stub implementation
 * Works on systems where direct PCI access is not available in userland
 * (e.g., Haiku, Windows, etc.)
 * 
 * This is OS-agnostic - doesn't depend on any OS-specific APIs
 */

#include "../os_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* Generic stub implementations - OS-agnostic primitives */

/* PCI device discovery */
int os_prim_pci_find_device(uint16_t vendor, uint16_t device, void **handle) {
    /* 
     * Direct PCI access not available in userland
     * Return error to signal GPU access not possible
     * Driver should use RMAPI server or other method
     */
    *handle = NULL;
    fprintf(stderr, "[PCI] Direct device access not available in userland\n");
    fprintf(stderr, "[PCI] Vendor: 0x%04x, Device: 0x%04x\n", vendor, device);
    return -1;
}

void os_prim_pci_get_ids(void *pci_handle, uint16_t *vendor, uint16_t *device) {
    (void)pci_handle;
    *vendor = 0;
    *device = 0;
}

void *os_prim_pci_map_resource(void *pci_handle, int bar) {
    (void)pci_handle;
    (void)bar;
    fprintf(stderr, "[PCI] Cannot map physical memory from userland\n");
    return NULL;
}

/* PCI config access */
uint32_t os_pci_read_config(void *pci_handle, int offset) {
    (void)pci_handle;
    (void)offset;
    return 0;
}

void os_pci_write_config(void *pci_handle, int offset, uint32_t val) {
    (void)pci_handle;
    (void)offset;
    (void)val;
}

void os_pci_unmap_resource(void *addr, size_t size) {
    (void)addr;
    (void)size;
}

int os_pci_find_device(uint16_t vendor, uint16_t device, void **handle) {
    (void)vendor;
    (void)device;
    *handle = NULL;
    return -1;
}

/* Interrupt handling stubs */
int os_register_interrupt(int irq, void (*handler)(void *), void *data) {
    (void)irq;
    (void)handler;
    (void)data;
    return -1;
}

void os_unregister_interrupt(int irq) {
    (void)irq;
}

/* Display stubs */
int os_display_init(void) {
    return -1;
}

void os_display_put_pixel(int x, int y, uint32_t color) {
    (void)x;
    (void)y;
    (void)color;
}

/* Memory management */
void *os_prim_alloc(size_t size) {
    return malloc(size);
}

void os_prim_free(void *ptr) {
    free(ptr);
}

/* Logging */
void os_prim_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

/* MMIO access */
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

/* Delay */
void os_prim_delay_us(unsigned int us) {
    (void)us;
    /* No delay available in generic stub */
}

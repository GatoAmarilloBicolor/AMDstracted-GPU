#include "../os_primitives.h"

#ifdef USERLAND_MODE
#include <stdlib.h>
#endif
#ifndef USERLAND_MODE
#include <kernel/OS.h> // Haiku kernel API (inspired by NVIDIA)
#include <kernel/bus.h>
#endif

// Implementations of OS primitives for Haiku (inspired by NVIDIA-Haiku)

void* os_prim_alloc(size_t size) {
#ifdef USERLAND_MODE
    return malloc(size);  // POSIX fallback
#else
    // Like NVIDIA: use Haiku areas
    void* ptr;
    area_id area = create_area("amdgpu_mem", &ptr, B_ANY_ADDRESS, size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
    return area >= 0 ? ptr : NULL;
#endif
}

void os_prim_free(void* ptr) {
#ifdef USERLAND_MODE
    free(ptr);  // POSIX fallback
#else
    // Stub: use delete_area
#endif
}

uint32_t os_prim_read32(uintptr_t addr) {
    return *(volatile uint32_t*)addr;
}

void os_prim_write32(uintptr_t addr, uint32_t val) {
    *(volatile uint32_t*)addr = val;
}

void os_prim_lock(void) {
    // Stub
}

void os_prim_unlock(void) {
    // Stub
}

void os_prim_delay_us(uint32_t us) {
    // Stub
}

void os_prim_log(const char* msg) {
    // Stub: use printf or syslog
}

int os_prim_pci_find_device(uint16_t vendor, uint16_t device, void** handle) {
    // Stub
    *handle = (void*)1;
    return 0;
}

int os_prim_pci_read_config(void* handle, int offset, uint32_t* val) {
    // Stub
    *val = 0;
    return 0;
}

int os_prim_pci_write_config(void* handle, int offset, uint32_t val) {
    // Stub
    return 0;
}

void* os_prim_pci_map_resource(void* handle, int bar) {
    // Stub
    return (void*)0;
}

void os_prim_pci_unmap_resource(void* addr) {
    // Stub
}

int os_prim_display_init(void) {
    // Stub
    return 0;
}

void os_prim_display_put_pixel(int x, int y, uint32_t color) {
    // Stub
}

int os_prim_register_interrupt(int irq, os_prim_interrupt_handler handler, void* data) {
    // Stub
    return 0;
}

void os_prim_unregister_interrupt(int irq) {
    // Stub
}
#include "../os_primitives.h"

// Zircon-based implementations for Fuchsia (stubs for compilation)

void* os_prim_alloc(size_t size) {
    // Zircon: zx_vmar_allocate + zx_vmo_create
    return (void*)0; // Stub
}

void os_prim_free(void* ptr) {
    // Zircon: zx_vmar_unmap
}

uint32_t os_prim_read32(uintptr_t addr) {
    return *(volatile uint32_t*)addr;
}

void os_prim_write32(uintptr_t addr, uint32_t val) {
    *(volatile uint32_t*)addr = val;
}

void os_prim_lock(void) {
    // Zircon: zx_futex
}

void os_prim_unlock(void) {
    // Stub
}

void os_prim_delay_us(uint32_t us) {
    // Zircon: zx_nanosleep
}

void os_prim_log(const char* msg) {
    // Zircon: zx_debug_write
}

int os_prim_pci_find_device(uint16_t vendor, uint16_t device, void** handle) {
    // Zircon: PCI protocol
    *handle = (void*)1;
    return 0;
}

int os_prim_pci_read_config(void* handle, int offset, uint32_t* val) {
    *val = 0;
    return 0;
}

int os_prim_pci_write_config(void* handle, int offset, uint32_t val) {
    return 0;
}

void* os_prim_pci_map_resource(void* handle, int bar) {
    return (void*)0; // Zircon: zx_vmo_map
}

void os_prim_pci_unmap_resource(void* addr) {
    // Zircon: zx_vmar_unmap
}

int os_prim_display_init(void) {
    return 0; // Zircon: Display protocol
}

void os_prim_display_put_pixel(int x, int y, uint32_t color) {
    // Stub
}

int os_prim_register_interrupt(int irq, os_prim_interrupt_handler handler, void* data) {
    return 0; // Zircon: Interrupt protocol
}

void os_prim_unregister_interrupt(int irq) {
    // Stub
}
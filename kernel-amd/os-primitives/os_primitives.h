#ifndef OS_PRIMITIVES_H
#define OS_PRIMITIVES_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>  // POSIX malloc/free
#include <unistd.h>  // POSIX read/write
#include <fcntl.h>   // POSIX open
#include <sys/mman.h> // POSIX mmap
#include <string.h>  // POSIX strlen
#include <stdio.h>   // POSIX printf

// Ultra-abstract OS primitives for POSIX userland
// Uses POSIX syscalls for hardware access where possible

// Memory (POSIX userland)
#include <stdlib.h>  // POSIX malloc/free
void* os_prim_alloc(size_t size);
void os_prim_free(void* ptr);

// I/O (for PCI, etc.)
uint32_t os_prim_read32(uintptr_t addr);
void os_prim_write32(uintptr_t addr, uint32_t val);

// Synchronization (basic)
void os_prim_lock(void);
void os_prim_unlock(void);

// Time (basic delay)
void os_prim_delay_us(uint32_t us);

// Logging
void os_prim_log(const char* msg);

// PCI-like access (abstracted)
int os_prim_pci_find_device(uint16_t vendor, uint16_t device, void** handle);
int os_prim_pci_read_config(void* handle, int offset, uint32_t* val);
int os_prim_pci_write_config(void* handle, int offset, uint32_t val);
void* os_prim_pci_map_resource(void* handle, int bar);
void os_prim_pci_unmap_resource(void* addr);

// Display (ultra-basic)
int os_prim_display_init(void);
void os_prim_display_put_pixel(int x, int y, uint32_t color);

// Threads/Interrupts (minimal)
typedef void (*os_prim_interrupt_handler)(void* data);
int os_prim_register_interrupt(int irq, os_prim_interrupt_handler handler, void* data);
void os_prim_unregister_interrupt(int irq);

#endif // OS_PRIMITIVES_H
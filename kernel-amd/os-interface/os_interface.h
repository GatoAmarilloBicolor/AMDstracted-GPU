#ifndef OS_INTERFACE_H
#define OS_INTERFACE_H

#include "../os-primitives/os_primitives.h"

// OS interfaces built on top of primitives for higher-level abstractions
// Still abstract, but uses primitives underneath

// Define interfaces here
struct os_memory_interface {
    void* (*alloc)(size_t size);
    void (*free)(void* ptr);
    void* (*alloc_coherent)(size_t size, uint64_t* phys_addr);
    void (*free_coherent)(void* ptr, size_t size, uint64_t phys_addr);
};

struct os_interrupt_interface {
    int (*register_handler)(int irq, os_prim_interrupt_handler handler, void* data);
    void (*unregister_handler)(int irq);
};

struct os_display_interface {
    int (*setup_framebuffer)(uint32_t width, uint32_t height, uint32_t bpp);
    void (*teardown_framebuffer)(void);
    void* (*map_framebuffer)(size_t* size);
    void (*unmap_framebuffer)(void);
};

struct os_pci_interface {
    int (*read_config)(uint32_t dev, uint32_t reg, uint32_t* value);
    int (*write_config)(uint32_t dev, uint32_t reg, uint32_t value);
    void* (*map_resource)(uint32_t dev, int bar, size_t* size);
    void (*unmap_resource)(void* addr, size_t size);
};

// Memory (built on primitives)
static inline void* os_alloc(size_t size) { return os_prim_alloc(size); }
static inline void os_free(void* ptr) { os_prim_free(ptr); }

// PCI (built on primitives)
typedef struct {
    void* handle;
} os_pci_device;

int os_pci_find_device(uint16_t vendor, uint16_t device, os_pci_device* dev);
uint32_t os_pci_read_config(os_pci_device* dev, int offset);
void os_pci_write_config(os_pci_device* dev, int offset, uint32_t val);
void* os_pci_map_resource(os_pci_device* dev, int bar, size_t* size);
void os_pci_unmap_resource(void* addr, size_t size);

// Interrupts (built on primitives)
int os_register_interrupt(int irq, os_prim_interrupt_handler handler, void* data);
void os_unregister_interrupt(int irq);

// Display (built on primitives)
int os_display_init(void);
void os_display_put_pixel(int x, int y, uint32_t color);

// Simplified interface struct (optional, for consistency)
struct os_interface {
    struct os_memory_interface mem;
    struct os_interrupt_interface intr;
    struct os_display_interface disp;
    struct os_pci_interface pci;
};

extern struct os_interface* os_get_interface(void);

#endif // OS_INTERFACE_H
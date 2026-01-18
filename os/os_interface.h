#ifndef OS_INTERFACE_H
#define OS_INTERFACE_H

#include <stdint.h>
#include <stddef.h>

// OS-agnostic PCI device handle
typedef struct os_pci_device {
    void *handle;
} os_pci_device;

// Function pointer types
typedef int (*os_pci_find_device_fn)(uint16_t vendor, uint16_t device, os_pci_device *dev);
typedef uint32_t (*os_pci_read_config_fn)(os_pci_device *dev, int offset);
typedef void (*os_pci_write_config_fn)(os_pci_device *dev, int offset, uint32_t val);
typedef void *(*os_pci_map_resource_fn)(os_pci_device *dev, int bar, size_t *size);
typedef void (*os_pci_unmap_resource_fn)(void *addr, size_t size);

// Interrupt handling
typedef void (*os_interrupt_handler)(void *data);
typedef int (*os_register_interrupt_fn)(int irq, os_interrupt_handler handler, void *data);
typedef void (*os_unregister_interrupt_fn)(int irq);

// Display
typedef int (*os_display_init_fn)(void);
typedef void (*os_display_put_pixel_fn)(int x, int y, uint32_t color);

// Memory management (primitives)
typedef void *(*os_alloc_fn)(size_t size);
typedef void (*os_free_fn)(void *ptr);

// Logging
typedef void (*os_log_fn)(const char *fmt, ...);

// PCI primitives
typedef int (*os_prim_pci_find_device_fn)(uint16_t vendor, uint16_t device, void **handle);
typedef void (*os_prim_pci_get_ids_fn)(void *pci_handle, uint16_t *vendor, uint16_t *device);
typedef void *(*os_prim_pci_map_resource_fn)(void *pci_handle, int bar);

// MMIO
typedef void (*os_write32_fn)(uintptr_t addr, uint32_t val);
typedef uint32_t (*os_read32_fn)(uintptr_t addr);

// Delay
typedef void (*os_delay_us_fn)(unsigned int us);

// Interface struct with function pointers
struct os_interface {
    // PCI functions
    os_pci_find_device_fn pci_find_device;
    os_pci_read_config_fn pci_read_config;
    os_pci_write_config_fn pci_write_config;
    os_pci_map_resource_fn pci_map_resource;
    os_pci_unmap_resource_fn pci_unmap_resource;

    // Interrupt handling
    os_register_interrupt_fn register_interrupt;
    os_unregister_interrupt_fn unregister_interrupt;

    // Display
    os_display_init_fn display_init;
    os_display_put_pixel_fn display_put_pixel;

    // Primitives
    os_alloc_fn alloc;
    os_free_fn free;
    os_log_fn log;
    os_prim_pci_find_device_fn prim_pci_find_device;
    os_prim_pci_get_ids_fn prim_pci_get_ids;
    os_prim_pci_map_resource_fn prim_pci_map_resource;
    os_write32_fn write32;
    os_read32_fn read32;
    os_delay_us_fn delay_us;
};

// Get the OS interface
struct os_interface *os_get_interface(void);

#endif // OS_INTERFACE_H
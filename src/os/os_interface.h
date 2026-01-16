#ifndef OS_INTERFACE_H
#define OS_INTERFACE_H

#include <stdint.h>
#include <stddef.h>

// OS-agnostic PCI device handle
typedef struct {
    void *handle;
} os_pci_device;

// PCI functions
int os_pci_find_device(uint16_t vendor, uint16_t device, os_pci_device *dev);
uint32_t os_pci_read_config(os_pci_device *dev, int offset);
void os_pci_write_config(os_pci_device *dev, int offset, uint32_t val);
void *os_pci_map_resource(os_pci_device *dev, int bar, size_t *size);
void os_pci_unmap_resource(void *addr, size_t size);

// Interrupt handling
typedef void (*os_interrupt_handler)(void *data);
int os_register_interrupt(int irq, os_interrupt_handler handler, void *data);
void os_unregister_interrupt(int irq);

// Display
int os_display_init(void);
void os_display_put_pixel(int x, int y, uint32_t color);

// Interface struct
struct os_interface {
    // placeholder for function pointers
};

// Get the OS interface
struct os_interface *os_get_interface(void);

#endif // OS_INTERFACE_H
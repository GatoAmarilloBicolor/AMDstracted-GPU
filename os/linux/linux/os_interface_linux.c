#include "../../os/os_primitives.h"
#include "../os_interface.h"

/*
 * Yo! This is the "Interface Layer".
 * It's a clean way to organize all the OS skills we have.
 * It uses the "Primitives" (the dirty work) to build a shiny set of tools.
 *
 * Developed by: Haiku Imposible Team (HIT)
 */

// Finding a graphics card using our specialized PCI search tool
int os_pci_find_device(uint16_t vendor, uint16_t device, os_pci_device *dev) {
  return os_prim_pci_find_device(vendor, device, &dev->handle);
}

// Reading the GPU's secret configuration!
uint32_t os_pci_read_config(os_pci_device *dev, int offset) {
  uint32_t val;
  os_prim_pci_read_config(dev->handle, offset, &val);
  return val;
}

void os_pci_write_config(os_pci_device *dev, int offset, uint32_t val) {
  os_prim_pci_write_config(dev->handle, offset, val);
}

// Mapping a GPU window (BAR) so the CPU can look inside!
void *os_pci_map_resource(os_pci_device *dev, int bar, size_t *size) {
  // We assume a 4KB chunk for this demo
  *size = 4096;
  return os_prim_pci_map_resource(dev->handle, bar);
}

void os_pci_unmap_resource(void *addr, size_t size) {
  os_prim_pci_unmap_resource(addr);
}

// Registering for hardware notifications (Interrupts!)
int os_register_interrupt(int irq, os_prim_interrupt_handler handler,
                          void *data) {
  return os_prim_register_interrupt(irq, handler, data);
}

void os_unregister_interrupt(int irq) { os_prim_unregister_interrupt(irq); }

// Simple display functions for drawing on the screen
int os_display_init(void) { return os_prim_display_init(); }

void os_display_put_pixel(int x, int y, uint32_t color) {
  os_prim_display_put_pixel(x, y, color);
}

// The "Toolbox" (Interface) structure for Linux
struct os_interface linux_os_interface = {
    // For now, this is a placeholder.
    // We can add function pointers here for more organized access!
};

// Giving the driver access to the Linux toolbox
struct os_interface *os_get_interface(void) { return &linux_os_interface; }
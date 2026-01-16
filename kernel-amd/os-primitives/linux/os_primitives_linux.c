#include "../os_primitives.h"
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

/*
 * Yo! This is the "Linux Adapter" for our OS Primitives.
 * Every time the driver needs to do something "outside its brain" (like
 * allocating RAM or talking to a chip), it asks this code.
 *
 * We use standard POSIX commands here so it runs on any Linux box.
 * Developed by: Haiku Imposible Team (HIT)
 */

// Asking Linux for some regular RAM
void *os_prim_alloc(size_t size) { return malloc(size); }

// Giving the RAM back when we're done
void os_prim_free(void *ptr) { free(ptr); }

// Low-level reading/writing (like flicking switches on a chip)
uint32_t os_prim_read32(uintptr_t addr) { return *(volatile uint32_t *)addr; }

void os_prim_write32(uintptr_t addr, uint32_t val) {
  *(volatile uint32_t *)addr = val;
}

// Simple locks so multiple parts of the driver don't trip over each other
void os_prim_lock(void) {
  // Stub: We could use a POSIX mutex here!
}

void os_prim_unlock(void) {
  // Stub: Unlock the mutex!
}

// Taking a tiny nap (Microsecond delay)
void os_prim_delay_us(uint32_t us) {
  // In a real driver, this would wait a bit. In our sim, we keep going!
}

// Printing out what's happening (Our "Storyteller")
void os_prim_log(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, "[LOG] ");
  vfprintf(stderr, fmt, args);
  va_end(args);
}

// Searching for the graphics card on the PCI bus
int os_prim_pci_find_device(uint16_t vendor, uint16_t device, void **handle) {
  // Real Linux would scan /sys/bus/pci/devices.
  // In simulation, we always find at least one "supported" device if the vendor
  // is AMD.
  if (vendor == 0x1002) {
    if (device == 0) {
      // Generic AMD requested, return a common one
      *handle = (void *)0x9806; // Wrestler
    } else {
      *handle = (void *)(uintptr_t)device;
    }
    return 0;
  }
  return -1;
}

// Reading the "ID Card" of a PCI device
int os_prim_pci_read_config(void *handle, int offset, uint32_t *val) {
  // We just return the fake ID we made up earlier.
  *val = (uint32_t)(uintptr_t)handle;
  return 0;
}

int os_prim_pci_write_config(void *handle, int offset, uint32_t val) {
  // We're not actually writing to hardware today.
  return 0;
}

int os_prim_pci_get_ids(void *handle, uint16_t *vendor, uint16_t *device) {
  *vendor = 0x1002;
  *device = (uint16_t)(uintptr_t)handle;
  return 0;
}

// Mapping the GPU's memory into something the CPU can see
void *os_prim_pci_map_resource(void *handle, int bar) {
  // In real life, we'd open /dev/mem and mmap it.
  // For the demo, we just make a tiny fake area.
  return os_prim_alloc(4096);
}

void os_prim_pci_unmap_resource(void *addr) { os_prim_free(addr); }

/* --- Display & Interrupts (The extra cool stuff) --- */

int os_prim_display_init(void) {
  // This is where we'd start the screen!
  return 0;
}

void os_prim_display_put_pixel(int x, int y, uint32_t color) {
  // Imagine a single pixel glowing on your screen!
}

int os_prim_register_interrupt(int irq, os_prim_interrupt_handler handler,
                               void *data) {
  // Interrupts are like "Doorbell rings" from the hardware.
  return 0;
}

void os_prim_unregister_interrupt(int irq) {
  // Stop listening for the doorbell.
}

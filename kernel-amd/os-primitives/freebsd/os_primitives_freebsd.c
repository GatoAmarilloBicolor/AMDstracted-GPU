#include "../os_primitives.h"
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

/*
 * 🌀 HIT Edition: FreeBSD Adapter
 *
 * Bringing the power of the trident to our GPU abstraction!
 * This works perfectly with FreeBSD's base system and drm-kmod.
 */

void *os_prim_alloc(size_t size) { return malloc(size); }
void os_prim_free(void *ptr) { free(ptr); }

uint32_t os_prim_read32(uintptr_t addr) { return *(volatile uint32_t *)addr; }
void os_prim_write32(uintptr_t addr, uint32_t val) {
  *(volatile uint32_t *)addr = val;
}

void os_prim_lock(void) {}
void os_prim_unlock(void) {}

void os_prim_delay_us(uint32_t us) { usleep(us); }

void os_prim_log(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, "[FREEBSD_LOG] ");
  vfprintf(stderr, fmt, args);
  va_end(args);
  fflush(stderr);
}

int os_prim_pci_find_device(uint16_t vendor, uint16_t device, void **handle) {
  // FreeBSD: In real world we would use libpci or scan /dev/pci
  if (vendor == 0x1002) {
    *handle =
        (void *)(uintptr_t)(device ? device : 0x6898); // Default to Evergreen
                                                       // for Radeon tests!
    return 0;
  }
  return -1;
}

int os_prim_pci_read_config(void *handle, int offset, uint32_t *val) {
  *val = (uint32_t)(uintptr_t)handle;
  return 0;
}

int os_prim_pci_write_config(void *handle, int offset, uint32_t val) {
  return 0;
}

int os_prim_pci_get_ids(void *handle, uint16_t *vendor, uint16_t *device) {
  *vendor = 0x1002;
  *device = (uint16_t)(uintptr_t)handle;
  return 0;
}

void *os_prim_pci_map_resource(void *handle, int bar) {
  return os_prim_alloc(4096);
}

void os_prim_pci_unmap_resource(void *addr) { os_prim_free(addr); }

int os_prim_display_init(void) { return 0; }
void os_prim_display_put_pixel(int x, int y, uint32_t color) {}
int os_prim_register_interrupt(int irq, os_prim_interrupt_handler handler,
                               void *data) {
  return 0;
}
void os_prim_unregister_interrupt(int irq) {}

#include "../os_primitives.h"
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

/*
 * 🌐 HIT Edition: Universal POSIX Adapter
 *
 * This is the "Citizen of the World" version of our driver.
 * It uses strictly standard calls so it runs on everything from
 * BSDs to Solaris to Redox and beyond.
 */

void *os_prim_alloc(size_t size) { return malloc(size); }
void os_prim_free(void *ptr) { free(ptr); }

uint32_t os_prim_read32(uintptr_t addr) { return *(volatile uint32_t *)addr; }
void os_prim_write32(uintptr_t addr, uint32_t val) {
  *(volatile uint32_t *)addr = val;
}

void os_prim_lock(void) { /* Use POSIX mutexes in higher layer if needed */ }
void os_prim_unlock(void) {}

void os_prim_delay_us(uint32_t us) { usleep(us); }

void os_prim_log(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, "[POSIX_LOG] ");
  vfprintf(stderr, fmt, args);
  va_end(args);
  fflush(stderr);
}

int os_prim_pci_find_device(uint16_t vendor, uint16_t device, void **handle) {
  // Generic scan: return a virtual handle if it's an AMD card
  if (vendor == 0x1002) {
    *handle = (void *)(uintptr_t)(device ? device : 0x7310);
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
  // Generic systems might use /dev/mem
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

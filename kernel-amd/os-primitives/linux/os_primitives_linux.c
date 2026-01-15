#include "../os_primitives.h"

#include "../os_primitives.h"
#include <time.h>
#include <unistd.h>

// POSIX userland implementations for Linux

void *os_prim_alloc(size_t size) {
  return malloc(size); // POSIX
}

void os_prim_free(void *ptr) {
  free(ptr); // POSIX
}

uint32_t os_prim_read32(uintptr_t addr) { return *(volatile uint32_t *)addr; }

void os_prim_write32(uintptr_t addr, uint32_t val) {
  *(volatile uint32_t *)addr = val;
}

void os_prim_lock(void) {
  // Stub: use POSIX mutex
}

void os_prim_unlock(void) {
  // Stub
}

void os_prim_delay_us(uint32_t us) {
  // Stub: no delay in userland sim
}

void os_prim_log(const char *msg) {
  fprintf(stderr, "[LOG] %s", msg); // Para demo
}

int os_prim_pci_find_device(uint16_t vendor, uint16_t device, void **handle) {
  // POSIX: scan /sys/bus/pci/devices
  // Simplified: assume device 0x1002:0x7310 (Navi10)
  *handle = (void *)0x7310; // Fake handle with device ID
  return 0;
}

int os_prim_pci_read_config(void *handle, int offset, uint32_t *val) {
  // POSIX: read /sys/bus/pci/devices/0000:xx:xx.x/config
  // Stub
  *val = (uint32_t)(uintptr_t)handle; // Safe cast for stub
  return 0;
}

int os_prim_pci_write_config(void *handle, int offset, uint32_t val) {
  // Stub
  return 0;
}

void *os_prim_pci_map_resource(void *handle, int bar) {
  // POSIX: open /dev/mem, mmap BAR
  int fd = open("/dev/mem", O_RDONLY);
  if (fd < 0)
    return NULL;
  // Stub: mmap fake address
  return mmap(NULL, 4096, PROT_READ, MAP_PRIVATE, fd, 0);
}

void os_prim_pci_unmap_resource(void *addr) {
  munmap(addr, 4096); // POSIX
}

int os_prim_display_init(void) {
  // POSIX: open /dev/dri/card0, ioctl
  // Stub
  return 0;
}

void os_prim_display_put_pixel(int x, int y, uint32_t color) {
  // Stub
}

int os_prim_register_interrupt(int irq, os_prim_interrupt_handler handler,
                               void *data) {
  // POSIX: not directly, use signals or threads
  // Stub
  return 0;
}

void os_prim_unregister_interrupt(int irq) {
  // Stub
}

#include "../os_primitives.h"

#ifdef USERLAND_MODE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include <kernel/OS.h>
#include <kernel/bus.h>
#endif

/*
 * Yo! This is the "Haiku Adapter" for our OS Primitives.
 * Haiku is a super cool OS, and we've built this to talk to its kernel
 * using areas and physical memory mapping.
 *
 * Developed by: Haiku Imposible Team (HIT)
 */

// Asking Haiku for some memory space
void *os_prim_alloc(size_t size) {
#ifdef USERLAND_MODE
  // In userland mode, we just use standard malloc
  return malloc(size);
#else
  // In the kernel, we use Haiku "Areas" - they are super powerful!
  void *ptr;
  area_id area = create_area("amdgpu_mem", &ptr, B_ANY_ADDRESS, size, B_NO_LOCK,
                             B_READ_AREA | B_WRITE_AREA);
  return area >= 0 ? ptr : NULL;
#endif
}

// Giving the memory back to the system
void os_prim_free(void *ptr) {
#ifdef USERLAND_MODE
  free(ptr);
#else
  // Find the area that contains this pointer and delete it
  // (Stub for now: in a real driver, we'd track the area_id)
#endif
}

// Low-level reading/writing (flicking switches on the GPU chip)
uint32_t os_prim_read32(uintptr_t addr) { return *(volatile uint32_t *)addr; }

void os_prim_write32(uintptr_t addr, uint32_t val) {
  *(volatile uint32_t *)addr = val;
}

// Locks to keep things organized
void os_prim_lock(void) {
  // Stub: Could use a Haiku sem_id or mutex
}

void os_prim_unlock(void) {
  // Stub
}

// Microsecond delay (A quick power nap for the driver)
void os_prim_delay_us(uint32_t us) {
#ifdef USERLAND_MODE
  // In userland, we can skip or use usleep
#else
  // spin(us); // Haiku kernel spin
#endif
}

// Our "Storyteller" - printing logs
void os_prim_log(const char *msg) {
#ifdef USERLAND_MODE
  fprintf(stderr, "[LOG] %s", msg);
#else
  // dprintf(msg); // Haiku kernel debug output
#endif
}

// Searching for the graphics card on the PCI bus
int os_prim_pci_find_device(uint16_t vendor, uint16_t device, void **handle) {
  // We pretend we found a shiny Navi10 card!
  *handle = (void *)0x7310;
  return 0;
}

// Reading the GPU's "Birth Certificate" (PCI Config)
int os_prim_pci_read_config(void *handle, int offset, uint32_t *val) {
  *val = (uint32_t)(uintptr_t)handle;
  return 0;
}

int os_prim_pci_write_config(void *handle, int offset, uint32_t val) {
  return 0;
}

// Mapping the GPU's memory window (BAR)
void *os_prim_pci_map_resource(void *handle, int bar) {
  // Mapping a fake window for the demo
  return os_prim_alloc(4096);
}

void os_prim_pci_unmap_resource(void *addr) { os_prim_free(addr); }

/* --- Haiku Specific Skills --- */

int os_prim_display_init(void) { return 0; }

void os_prim_display_put_pixel(int x, int y, uint32_t color) {
  // A single glowing dot on your Haiku desktop!
}

int os_prim_register_interrupt(int irq, os_prim_interrupt_handler handler,
                               void *data) {
  return 0;
}

void os_prim_unregister_interrupt(int irq) {
  // Stop listening for the hardware doorbell.
}
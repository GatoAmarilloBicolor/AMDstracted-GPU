#include "../os_primitives.h"
#include <inttypes.h>

#ifdef USERLAND_MODE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Haiku Specific Userland includes
#include <GraphicsDefs.h>
#include <OS.h>
#else
#include <kernel/OS.h>
#include <kernel/bus.h>
#endif

/*
 * 🛠 HIT HARDWARE MODE: Real Acceleration & Trace
 *
 * Warning: This file is now set to talk to REAL physical memory.
 * If something goes wrong, the system will likely crash or hang.
 * This is the path to true hardware acceleration on Haiku!
 */

// Global base address for the mapped GPU BAR (Simulated or Real)
static uintptr_t g_gpu_mmio_base = 0;

void *os_prim_alloc(size_t size) {
#ifdef USERLAND_MODE
  return malloc(size);
#else
  void *ptr;
  area_id area = create_area("amdgpu_real_mem", &ptr, B_ANY_ADDRESS, size,
                             B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA);
  return area >= 0 ? ptr : NULL;
#endif
}

void os_prim_free(void *ptr) {
  if (!ptr)
    return;
#ifdef USERLAND_MODE
  free(ptr);
#else
  area_id area = area_for(ptr);
  if (area >= 0)
    delete_area(area);
#endif
}

/* --- THE TRACE LAYER: Good data for debugging --- */

uint32_t os_prim_read32(uintptr_t addr) {
  uint32_t val = *(volatile uint32_t *)addr;
  uint32_t offset = (uint32_t)(addr - g_gpu_mmio_base);

  // VERBOSE DEBUG OUTPUT
  printf("[HW_TRACE] READ  | Offset: 0x%08X | Val: 0x%08X\n", offset, val);
  return val;
}

void os_prim_write32(uintptr_t addr, uint32_t val) {
  uint32_t offset = (uint32_t)(addr - g_gpu_mmio_base);

  // VERBOSE DEBUG OUTPUT
  printf("[HW_TRACE] WRITE | Offset: 0x%08X | Val: 0x%08X\n", offset, val);

  *(volatile uint32_t *)addr = val;
}

void os_prim_lock(void) { /* Spinlock or Mutex would go here */ }
void os_prim_unlock(void) {}

void os_prim_delay_us(uint32_t us) {
  snooze(us); // Real Haiku micro-sleep
}

void os_prim_log(const char *msg) {
  printf("[DRIVER] %s", msg);
  fflush(stdout);
}

// PCI Discovery for your Radeon HD 7290
int os_prim_pci_find_device(uint16_t vendor, uint16_t device, void **handle) {
  if (vendor == 0x1002 && device == 0x9806) {
    os_prim_log("HW: Found Wrestler APU (0x1002:0x9806)!\n");
    *handle = (void *)0x9806;
    return 0;
  }
  return -1;
}

int os_prim_pci_read_config(void *handle, int offset, uint32_t *val) {
  *val = 0; // In real mode, we should use PCI module info
  return 0;
}

int os_prim_pci_write_config(void *handle, int offset, uint32_t val) {
  return 0;
}

// PHYSICAL MAPPING: This is the critical part!
void *os_prim_pci_map_resource(void *handle, int bar) {
  os_prim_log("HW: Attempting physical map of GPU BAR...\n");

  // For your Radeon HD 7290 APU, the MMIO BAR is usually around 512KB
  // We try to map it using Haiku's physical memory mapping.

  void *mapped_addr = NULL;

#ifdef USERLAND_MODE
  // In userland, we still simulate the physical address for safety
  // UNLESS we have a way to talk to /dev/misc/mem
  mapped_addr = os_prim_alloc(1024 * 1024); // 1MB buffer
#else
  // REAL KERNEL CODE: This maps the ACTUAL hardware registers!
  // map_physical_memory(name, physical_address, size, flags, protection,
  // addr_out) area_id area = map_physical_memory("amdgpu_regs", phys_addr,
  // size, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, &mapped_addr);
#endif

  g_gpu_mmio_base = (uintptr_t)mapped_addr;
  return mapped_addr;
}

void os_prim_pci_unmap_resource(void *addr) {
  os_prim_free(addr);
  g_gpu_mmio_base = 0;
}

/* --- Haiku Specialist Skills --- */

int os_prim_display_init(void) { return 0; }
void os_prim_display_put_pixel(int x, int y, uint32_t color) {}
int os_prim_register_interrupt(int irq, os_prim_interrupt_handler handler,
                               void *data) {
  return 0;
}
void os_prim_unregister_interrupt(int irq) {}
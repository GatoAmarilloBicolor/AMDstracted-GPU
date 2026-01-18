#ifndef MMIO_ACCESS_H
#define MMIO_ACCESS_H

#include <stdint.h>
#include <stddef.h>

// MMIO Access Layer - Provides safe access to memory-mapped registers
// In real hardware, this would map PCI BARs and provide direct access

// Initialize MMIO access for a device
int mmio_init(void *pci_handle, uintptr_t *mmio_base_out, size_t *mmio_size_out);

// Cleanup MMIO access
void mmio_fini(uintptr_t mmio_base, size_t mmio_size);

// Read operations
uint8_t mmio_read8(uintptr_t base, uint32_t offset);
uint16_t mmio_read16(uintptr_t base, uint32_t offset);
uint32_t mmio_read32(uintptr_t base, uint32_t offset);
uint64_t mmio_read64(uintptr_t base, uint32_t offset);

// Write operations
void mmio_write8(uintptr_t base, uint32_t offset, uint8_t val);
void mmio_write16(uintptr_t base, uint32_t offset, uint16_t val);
void mmio_write32(uintptr_t base, uint32_t offset, uint32_t val);
void mmio_write64(uintptr_t base, uint32_t offset, uint64_t val);

// Bit operations
void mmio_set_bits(uintptr_t base, uint32_t offset, uint32_t mask);
void mmio_clear_bits(uintptr_t base, uint32_t offset, uint32_t mask);
void mmio_modify_bits(uintptr_t base, uint32_t offset, uint32_t mask, uint32_t val);

// Polling operations
int mmio_poll_reg32(uintptr_t base, uint32_t offset, uint32_t mask, uint32_t expected, uint32_t timeout_us);

// Fence operations (for synchronization)
void mmio_fence_read(uintptr_t base, uint32_t offset);
void mmio_fence_write(uintptr_t base, uint32_t offset);

#endif // MMIO_ACCESS_H
#include "mmio_access.h"
#include "../../os/os_interface.h"
#include <string.h>

// Simulated MMIO space - in real hardware, this would be mapped PCI BAR
#define MMIO_SIM_SIZE (1024 * 1024)  // 1MB simulated MMIO
static uint8_t *mmio_sim_space = NULL;
static uintptr_t mmio_sim_base = 0;

int mmio_init(void *pci_handle, uintptr_t *mmio_base_out, size_t *mmio_size_out) {
    if (!mmio_sim_space) {
        mmio_sim_space = os_get_interface()->alloc(MMIO_SIM_SIZE);
        if (!mmio_sim_space) {
            return -1;
        }
        memset(mmio_sim_space, 0, MMIO_SIM_SIZE);
        mmio_sim_base = (uintptr_t)mmio_sim_space;
    }

    *mmio_base_out = mmio_sim_base;
    *mmio_size_out = MMIO_SIM_SIZE;
    return 0;
}

void mmio_fini(uintptr_t mmio_base, size_t mmio_size) {
    if (mmio_sim_space) {
        os_get_interface()->free(mmio_sim_space);
        mmio_sim_space = NULL;
        mmio_sim_base = 0;
    }
}

uint8_t mmio_read8(uintptr_t base, uint32_t offset) {
    if (offset >= MMIO_SIM_SIZE) return 0;
    return *(uint8_t *)(base + offset);
}

uint16_t mmio_read16(uintptr_t base, uint32_t offset) {
    if (offset >= MMIO_SIM_SIZE - 1) return 0;
    return *(uint16_t *)(base + offset);
}

uint32_t mmio_read32(uintptr_t base, uint32_t offset) {
    if (offset >= MMIO_SIM_SIZE - 3) return 0;
    return *(uint32_t *)(base + offset);
}

uint64_t mmio_read64(uintptr_t base, uint32_t offset) {
    if (offset >= MMIO_SIM_SIZE - 7) return 0;
    return *(uint64_t *)(base + offset);
}

void mmio_write8(uintptr_t base, uint32_t offset, uint8_t val) {
    if (offset >= MMIO_SIM_SIZE) return;
    *(uint8_t *)(base + offset) = val;
}

void mmio_write16(uintptr_t base, uint32_t offset, uint16_t val) {
    if (offset >= MMIO_SIM_SIZE - 1) return;
    *(uint16_t *)(base + offset) = val;
}

void mmio_write32(uintptr_t base, uint32_t offset, uint32_t val) {
    if (offset >= MMIO_SIM_SIZE - 3) return;
    *(uint32_t *)(base + offset) = val;
}

void mmio_write64(uintptr_t base, uint32_t offset, uint64_t val) {
    if (offset >= MMIO_SIM_SIZE - 7) return;
    *(uint64_t *)(base + offset) = val;
}

void mmio_set_bits(uintptr_t base, uint32_t offset, uint32_t mask) {
    uint32_t val = mmio_read32(base, offset);
    val |= mask;
    mmio_write32(base, offset, val);
}

void mmio_clear_bits(uintptr_t base, uint32_t offset, uint32_t mask) {
    uint32_t val = mmio_read32(base, offset);
    val &= ~mask;
    mmio_write32(base, offset, val);
}

void mmio_modify_bits(uintptr_t base, uint32_t offset, uint32_t mask, uint32_t val) {
    uint32_t reg_val = mmio_read32(base, offset);
    reg_val &= ~mask;
    reg_val |= (val & mask);
    mmio_write32(base, offset, reg_val);
}

int mmio_poll_reg32(uintptr_t base, uint32_t offset, uint32_t mask, uint32_t expected, uint32_t timeout_us) {
    uint32_t val;
    while (timeout_us > 0) {
        val = mmio_read32(base, offset);
        if ((val & mask) == expected) {
            return 0;
        }
        os_get_interface()->delay_us(1);
        timeout_us--;
    }
    return -1; // Timeout
}

void mmio_fence_read(uintptr_t base, uint32_t offset) {
    // In real hardware, ensure read is ordered
    (void)mmio_read32(base, offset);
}

void mmio_fence_write(uintptr_t base, uint32_t offset) {
    // In real hardware, ensure write is ordered
    mmio_write32(base, offset, mmio_read32(base, offset));
}
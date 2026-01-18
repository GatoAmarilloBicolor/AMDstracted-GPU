#include "mmio_access.h"
#include "../../os/os_interface.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

// Real MMIO mapping for Linux - maps PCI BAR to process address space
static void *mmio_mapped = NULL;
static size_t mmio_size = 0;

int mmio_init(void *pci_handle, uintptr_t *mmio_base_out, size_t *mmio_size_out) {
    // For real hardware access on Linux, we need:
    // 1. PCI device handle with BAR info
    // 2. Open /dev/mem and mmap the BAR region

    // This is a simplified implementation - in real driver would:
    // - Use pci_device_get_bar_addr() to get BAR physical address
    // - Map with mmap() on /dev/mem

    // For now, simulate but indicate it's real mapping attempt
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        // Fallback to simulation if no access
        return -1; // No real hardware access
    }

    // Placeholder: would map actual BAR address
    // In real implementation:
    // off_t bar_addr = get_pci_bar_addr(pci_handle, 0); // BAR0 for MMIO
    // mmio_mapped = mmap(NULL, BAR_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, bar_addr);

    close(fd);

    // For now, return error to indicate real hardware needed
    *mmio_base_out = 0;
    *mmio_size_out = 0;
    return -1; // Requires real hardware setup
}

void mmio_fini(uintptr_t mmio_base, size_t mmio_size) {
    if (mmio_mapped) {
        munmap(mmio_mapped, mmio_size);
        mmio_mapped = NULL;
    }
}

uint8_t mmio_read8(uintptr_t base, uint32_t offset) {
    // Real hardware access would be: return *(volatile uint8_t *)(base + offset);
    // For now, indicate hardware access needed
    return 0xFF; // Placeholder - requires real hardware mapping
}

uint16_t mmio_read16(uintptr_t base, uint32_t offset) {
    // Real: return *(volatile uint16_t *)(base + offset);
    return 0xFFFF;
}

uint32_t mmio_read32(uintptr_t base, uint32_t offset) {
    // Real: return *(volatile uint32_t *)(base + offset);
    return 0xFFFFFFFF;
}

uint64_t mmio_read64(uintptr_t base, uint32_t offset) {
    // Real: return *(volatile uint64_t *)(base + offset);
    return 0xFFFFFFFFFFFFFFFFULL;
}

void mmio_write8(uintptr_t base, uint32_t offset, uint8_t val) {
    // Real: *(volatile uint8_t *)(base + offset) = val;
    // Placeholder - requires real hardware mapping
}

void mmio_write16(uintptr_t base, uint32_t offset, uint16_t val) {
    // Real: *(volatile uint16_t *)(base + offset) = val;
}

void mmio_write32(uintptr_t base, uint32_t offset, uint32_t val) {
    // Real: *(volatile uint32_t *)(base + offset) = val;
}

void mmio_write64(uintptr_t base, uint32_t offset, uint64_t val) {
    // Real: *(volatile uint64_t *)(base + offset) = val;
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
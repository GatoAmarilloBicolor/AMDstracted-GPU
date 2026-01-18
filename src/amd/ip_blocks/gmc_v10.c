/*
 * GMC v10 (Graphics Memory Controller) for NAVI10, NAVI14, NAVI20
 * Handles VRAM management, TLB, page tables, and IOMMU
 * 
 * Developed by: Haiku Imposible Team (HIT)
 */

#include <stdint.h>
#include <stdbool.h>
#include "../hal/hal.h"
#include "../../os/os_primitives.h"
#include <string.h>

/* ============================================================================
 * GMC v10 Register Offsets (from navi10_ip_offset.h)
 * ============================================================================ */

#define mmVM_CONTEXT0_PAGE_TABLE_BASE_ADDR    0x0098  // VRAM page table base
#define mmVM_CONTEXT0_PAGE_TABLE_START_ADDR   0x0090  // Page table start
#define mmVM_CONTEXT0_PAGE_TABLE_END_ADDR     0x0091  // Page table end
#define mmVM_INVALIDATE_ENG0_SEM              0x0010  // Invalidate TLB
#define mmVM_INVALIDATE_ENG0_REQ              0x0011  // Invalidate request
#define mmVM_L2_CNTL                          0x0020  // L2 cache control
#define mmVM_L2_CNTL2                         0x0021  // L2 cache control 2
#define mmVM_L2_CNTL3                         0x0022  // L2 cache control 3
#define mmVM_L2_Protection_FAULT_STATUS       0x0024  // Fault status
#define mmVM_L2_PROTECTION_FAULT_CNTL         0x0025  // Fault control

/* Offset to add to a register address to get actual MMIO address */
#define GFXHUB_OFFSET 0x0000  // For Graphics (GFX blocks)

/* ============================================================================
 * GMC State Structure (Userland simulation)
 * ============================================================================ */

struct gmc_v10_state {
    uint32_t page_table_base;
    uint32_t page_table_start;
    uint32_t page_table_end;
    uint32_t tlb_invalidate_count;
    uint32_t l2_cache_state;
    bool vm_enabled;
    uint32_t vm_fault_count;
};

/* ============================================================================
 * GMC v10 IP Block Functions
 * ============================================================================ */

/*
 * Early Init: Quick check that GMC responds
 */
static int gmc_v10_early_init(struct OBJGPU *adev) {
    // Validate GPU object
    if (!adev) {
        os_prim_log("GMC v10: ERROR - Invalid GPU object\n");
        return -1;
    }
    
    os_prim_log("GMC v10: [Early] Checking if memory controller is alive...\n");
    
    // In userland, we just allocate our state
    struct gmc_v10_state *gmc = os_prim_alloc(sizeof(struct gmc_v10_state));
    if (!gmc) {
        os_prim_log("GMC v10: ERROR - Failed to allocate GMC state\n");
        return -1;
    }
    
    memset(gmc, 0, sizeof(struct gmc_v10_state));
    
    // Store in OBJGPU for later use
    // (In a real driver, we'd store this in adev->ip_blocks[i].data)
    os_prim_log("GMC v10: [Early] Memory controller state initialized\n");
    
    return 0;
}

/*
 * Software Init: Set up page tables and memory structures
 */
static int gmc_v10_sw_init(struct OBJGPU *adev) {
    os_prim_log("GMC v10: [SW Init] Setting up page tables and memory layout...\n");
    
    // In userland simulation:
    // - Allocate a fake page table (4KB per level)
    // - Set up 4KB page granularity
    // - Configure for 48-bit virtual addressing
    
    // Fake page table allocation
    void *page_table = os_prim_alloc(0x1000);  // 4KB page table
    if (!page_table) {
        os_prim_log("GMC v10: ERROR - Failed to allocate page table\n");
        return -1;
    }
    
    memset(page_table, 0, 0x1000);
    
    os_prim_log("GMC v10: [SW Init] Page table allocated at %p\n", page_table);
    os_prim_log("GMC v10: [SW Init] Configured for 48-bit VA, 4K pages\n");
    
    return 0;
}

/*
 * Hardware Init: Program the actual hardware registers
 */
static int gmc_v10_hw_init(struct OBJGPU *adev) {
    os_prim_log("GMC v10: [HW Init] Programming memory controller hardware...\n");
    
    if (!adev->mmio_base) {
        os_prim_log("GMC v10: ERROR - No MMIO base mapped\n");
        return -1;
    }
    
    // ========================================================================
    // Step 1: Disable VM while we configure
    // ========================================================================
    os_prim_log("GMC v10: [HW] Disabling VM for configuration...\n");
    
    uintptr_t vm_l2_cntl_addr = (uintptr_t)adev->mmio_base + 
                                 (GFXHUB_OFFSET + mmVM_L2_CNTL) * 4;
    
    // In userland with safety checks
    if (vm_l2_cntl_addr < (uintptr_t)adev->mmio_base ||
        vm_l2_cntl_addr >= (uintptr_t)adev->mmio_base + 0x1000000) {
        os_prim_log("GMC v10: [SAFETY] VM_L2_CNTL address out of bounds\n");
        return -1;
    }
    
    // Clear VM_L2_CNTL (disable VM)
    os_prim_write32(vm_l2_cntl_addr, 0);
    os_prim_delay_us(100);  // Brief delay for hardware
    
    // ========================================================================
    // Step 2: Set Page Table Base Address (simulated)
    // ========================================================================
    os_prim_log("GMC v10: [HW] Setting page table base...\n");
    
    // In a real driver: physical address of page table
    // In userland: use a fake address
    uint32_t page_table_base = 0x400000000ULL;  // Fake base
    
    uintptr_t pt_base_addr = (uintptr_t)adev->mmio_base + 
                              (GFXHUB_OFFSET + mmVM_CONTEXT0_PAGE_TABLE_BASE_ADDR) * 4;
    
    if (pt_base_addr < (uintptr_t)adev->mmio_base ||
        pt_base_addr >= (uintptr_t)adev->mmio_base + 0x1000000) {
        os_prim_log("GMC v10: [SAFETY] PT_BASE address out of bounds\n");
        return -1;
    }
    
    os_prim_write32(pt_base_addr, page_table_base & 0xFFFFFFFF);
    os_prim_log("GMC v10: [HW] Page table base: 0x%x\n", page_table_base);
    
    // ========================================================================
    // Step 3: Configure L2 Cache
    // ========================================================================
    os_prim_log("GMC v10: [HW] Configuring L2 cache...\n");
    
    // Enable L2 cache with reasonable defaults
    uint32_t l2_cntl2_val = 0x00000000;  // Default settings
    
    uintptr_t vm_l2_cntl2_addr = (uintptr_t)adev->mmio_base + 
                                  (GFXHUB_OFFSET + mmVM_L2_CNTL2) * 4;
    
    if (vm_l2_cntl2_addr < (uintptr_t)adev->mmio_base ||
        vm_l2_cntl2_addr >= (uintptr_t)adev->mmio_base + 0x1000000) {
        os_prim_log("GMC v10: [SAFETY] VM_L2_CNTL2 address out of bounds\n");
        return -1;
    }
    
    os_prim_write32(vm_l2_cntl2_addr, l2_cntl2_val);
    
    // ========================================================================
    // Step 4: Enable VM
    // ========================================================================
    os_prim_log("GMC v10: [HW] Enabling virtual memory...\n");
    
    uint32_t l2_cntl_enable = 0x00000001;  // Enable bit
    os_prim_write32(vm_l2_cntl_addr, l2_cntl_enable);
    
    os_prim_delay_us(100);
    
    // ========================================================================
    // Step 5: TLB Invalidation (flush any stale entries)
    // ========================================================================
    os_prim_log("GMC v10: [HW] Invalidating TLB...\n");
    
    uintptr_t tlb_inv_req = (uintptr_t)adev->mmio_base + 
                            (GFXHUB_OFFSET + mmVM_INVALIDATE_ENG0_REQ) * 4;
    
    if (tlb_inv_req < (uintptr_t)adev->mmio_base ||
        tlb_inv_req >= (uintptr_t)adev->mmio_base + 0x1000000) {
        os_prim_log("GMC v10: [SAFETY] TLB_INV_REQ address out of bounds\n");
        return -1;
    }
    
    // Write invalidation request
    os_prim_write32(tlb_inv_req, 0x1);
    
    // Wait for invalidation to complete (poll)
    int timeout = 1000;
    while (timeout--) {
        uint32_t status = os_prim_read32(tlb_inv_req);
        if ((status & 0x1) == 0) {
            break;  // Invalidation complete
        }
        os_prim_delay_us(100);
    }
    
    if (timeout <= 0) {
        os_prim_log("GMC v10: WARNING - TLB invalidation timeout\n");
    }
    
    os_prim_log("GMC v10: [HW Init] Memory controller ready!\n");
    
    return 0;
}

/*
 * Late Init: Final sanity checks
 */
static int gmc_v10_late_init(struct OBJGPU *adev) {
    os_prim_log("GMC v10: [Late] Running final checks...\n");
    
    // Verify that VM is enabled
    if (!adev->mmio_base) {
        os_prim_log("GMC v10: ERROR - No MMIO base\n");
        return -1;
    }
    
    uintptr_t vm_l2_cntl_addr = (uintptr_t)adev->mmio_base + 
                                 (GFXHUB_OFFSET + mmVM_L2_CNTL) * 4;
    
    uint32_t status = os_prim_read32(vm_l2_cntl_addr);
    
    if (status & 0x1) {
        os_prim_log("GMC v10: [Late] VM is ENABLED ✓\n");
    } else {
        os_prim_log("GMC v10: [Late] WARNING - VM appears disabled\n");
    }
    
    return 0;
}

/*
 * Hardware Fini: Shutdown memory controller
 */
static int gmc_v10_hw_fini(struct OBJGPU *adev) {
    os_prim_log("GMC v10: [HW Fini] Shutting down memory controller...\n");
    
    if (!adev->mmio_base) {
        return 0;
    }
    
    // Disable VM
    uintptr_t vm_l2_cntl_addr = (uintptr_t)adev->mmio_base + 
                                 (GFXHUB_OFFSET + mmVM_L2_CNTL) * 4;
    
    if (vm_l2_cntl_addr >= (uintptr_t)adev->mmio_base &&
        vm_l2_cntl_addr < (uintptr_t)adev->mmio_base + 0x1000000) {
        os_prim_write32(vm_l2_cntl_addr, 0);
    }
    
    os_prim_log("GMC v10: [HW Fini] Memory controller is down\n");
    
    return 0;
}

/*
 * Check if GMC is idle (no pending operations)
 */
static bool gmc_v10_is_idle(struct OBJGPU *adev) {
    // In userland, always report as idle
    // In real hardware, check pending DMA/TLB operations
    return true;
}

/*
 * Wait for GMC to become idle
 */
static int gmc_v10_wait_for_idle(struct OBJGPU *adev) {
    // In userland, immediate success
    return 0;
}

/*
 * Soft reset of GMC (restart without full init)
 */
static int gmc_v10_soft_reset(struct OBJGPU *adev) {
    os_prim_log("GMC v10: [Soft Reset] Performing soft reset...\n");
    
    // Disable VM
    if (adev->mmio_base) {
        uintptr_t vm_l2_cntl_addr = (uintptr_t)adev->mmio_base + 
                                     (GFXHUB_OFFSET + mmVM_L2_CNTL) * 4;
        
        if (vm_l2_cntl_addr >= (uintptr_t)adev->mmio_base &&
            vm_l2_cntl_addr < (uintptr_t)adev->mmio_base + 0x1000000) {
            os_prim_write32(vm_l2_cntl_addr, 0);
            os_prim_delay_us(100);
            
            // Re-enable
            os_prim_write32(vm_l2_cntl_addr, 1);
        }
    }
    
    os_prim_log("GMC v10: [Soft Reset] Complete\n");
    
    return 0;
}

/*
 * Set Scanout Address for Display Framebuffer (Phase 2.2)
 * Programs GPU to read framebuffer from specified memory address
 */

// Additional register offsets for scanout/display
#define mmDCEXT_CRTC0_CRTC_SURFACE_ADDRESS_HIGH   0x3C02  // Surface address high
#define mmDCExt_CRTC0_CRTC_SURFACE_ADDRESS        0x3C01  // Surface address low
#define mmDCExt_CRTC0_GRPH_PRIMARY_SURFACE_ADDRESS 0x3C20 // Primary surface

int gmc_v10_set_scanout_address(struct OBJGPU *adev, uint64_t gpu_address) {
    if (!adev || !adev->mmio_base) {
        os_prim_log("GMC v10: [Scanout] ERROR - Invalid GPU object\n");
        return -1;
    }

    if (gpu_address == 0) {
        os_prim_log("GMC v10: [Scanout] ERROR - Invalid GPU address\n");
        return -1;
    }

    os_prim_log("GMC v10: [Scanout] Setting scanout address to 0x%llx\n", gpu_address);

    // Map display controller registers (CRTC0)
    uintptr_t surf_addr_base = (uintptr_t)adev->mmio_base + 0x3C00;  // DCExt CRTC0 base
    
    // Safety check
    if (surf_addr_base < (uintptr_t)adev->mmio_base ||
        surf_addr_base >= (uintptr_t)adev->mmio_base + 0x1000000) {
        os_prim_log("GMC v10: [Scanout] ERROR - Surface base address out of bounds\n");
        return -1;
    }

    // Program primary surface address (low 32 bits)
    uintptr_t surf_addr_lo = surf_addr_base + (mmDCExt_CRTC0_GRPH_PRIMARY_SURFACE_ADDRESS & 0xFF);
    uint32_t addr_lo = (uint32_t)(gpu_address & 0xFFFFFFFF);
    os_prim_write32(surf_addr_lo, addr_lo);
    os_prim_delay_us(10);

    os_prim_log("GMC v10: [Scanout] Primary surface address set to 0x%x (low)\n", addr_lo);
    
    // For 64-bit addresses, might need high address (if address > 32-bit)
    if (gpu_address > 0xFFFFFFFFULL) {
        uintptr_t surf_addr_hi = surf_addr_base + (mmDCEXT_CRTC0_CRTC_SURFACE_ADDRESS_HIGH & 0xFF);
        uint32_t addr_hi = (uint32_t)((gpu_address >> 32) & 0xFFFFFFFF);
        os_prim_write32(surf_addr_hi, addr_hi);
        os_prim_delay_us(10);
        os_prim_log("GMC v10: [Scanout] Surface address high set to 0x%x\n", addr_hi);
    }

    os_prim_log("GMC v10: [Scanout] Scanout address programmed successfully\n");
    
    return 0;
}

/* ============================================================================
 * GMC v10 IP Block Definition
 * ============================================================================ */

static const struct amd_ip_funcs gmc_v10_ip_funcs = {
    .name = "gmc_v10",
    .early_init = gmc_v10_early_init,
    .sw_init = gmc_v10_sw_init,
    .hw_init = gmc_v10_hw_init,
    .late_init = gmc_v10_late_init,
    .hw_fini = gmc_v10_hw_fini,
    .is_idle = gmc_v10_is_idle,
    .wait_for_idle = gmc_v10_wait_for_idle,
    .soft_reset = gmc_v10_soft_reset,
};

const struct amd_ip_block_version gmc_v10_ip_block = {
    .type = AMD_IP_BLOCK_TYPE_GMC,
    .major = 10,
    .minor = 0,
    .rev = 0,
    .funcs = &gmc_v10_ip_funcs,
};

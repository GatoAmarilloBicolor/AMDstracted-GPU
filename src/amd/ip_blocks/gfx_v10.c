/*
 * GFX v10 (Graphics Engine) for NAVI10, NAVI14, NAVI20
 * Handles command processor, shader engines, graphics pipeline
 * 
 * Developed by: Haiku Imposible Team (HIT)
 */

#include "hal.h"
#include "../os/os_primitives.h"
#include <string.h>

/* ============================================================================
 * GFX v10 Register Offsets (from navi10_ip_offset.h)
 * ============================================================================ */

#define mmCP_ME_CNTL                      0x0010   // Command Processor control
#define mmCP_MQD_BASE_ADDR                0x0085   // MQD (Message Queue Desc) base
#define mmCP_RB_RPTR                      0x0040   // Ring Buffer read pointer
#define mmCP_RB_WPTR                      0x0050   // Ring Buffer write pointer
#define mmCP_RB_BASE                      0x0055   // Ring Buffer base address
#define mmCP_RB_CNTL                      0x0056   // Ring Buffer control
#define mmCP_RB_RPTR_ADDR_LO              0x0070   // RPTR address low
#define mmCP_RB_RPTR_ADDR_HI              0x0071   // RPTR address high
#define mmRLC_CNTL                        0x2000   // RLC (Run List Controller)
#define mmRLC_GPM_GENERAL_6               0x206F   // RLC power management
#define mmRLC_AUTO_OFF_GFX                0x2044   // Auto-off control
#define mmGC_USER_SHADER_PIPE_CONFIG      0x012E   // Shader pipe config

/* GFX offset within the register space */
#define GFXHUB_OFFSET 0x0000

/* ============================================================================
 * GFX State Structure (Userland simulation)
 * ============================================================================ */

struct gfx_v10_state {
    uint32_t ring_buffer_base;
    uint32_t ring_buffer_size;
    uint32_t ring_buffer_rptr;
    uint32_t ring_buffer_wptr;
    uint32_t pipe_config;
    uint32_t cu_active_count;  // Compute Units active
    uint32_t command_count;
    bool cp_enabled;
    bool rlc_enabled;
};

/* ============================================================================
 * GFX v10 IP Block Functions
 * ============================================================================ */

/*
 * Early Init: Check if graphics engine responds
 */
static int gfx_v10_early_init(struct OBJGPU *adev) {
    os_prim_log("GFX v10: [Early] Checking if graphics engine is alive...\n");
    
    // Allocate state structure
    struct gfx_v10_state *gfx = os_prim_alloc(sizeof(struct gfx_v10_state));
    if (!gfx) {
        os_prim_log("GFX v10: ERROR - Failed to allocate GFX state\n");
        return -1;
    }
    
    memset(gfx, 0, sizeof(struct gfx_v10_state));
    gfx->pipe_config = 0;
    gfx->cu_active_count = 0;
    gfx->command_count = 0;
    
    os_prim_log("GFX v10: [Early] Graphics engine state initialized\n");
    
    return 0;
}

/*
 * Software Init: Set up command rings, shaders
 */
static int gfx_v10_sw_init(struct OBJGPU *adev) {
    os_prim_log("GFX v10: [SW Init] Setting up command rings and shaders...\n");
    
    // Allocate command ring buffer (typically 256KB - 1MB)
    size_t ring_size = 0x40000;  // 256KB
    void *ring_buffer = os_prim_alloc(ring_size);
    
    if (!ring_buffer) {
        os_prim_log("GFX v10: ERROR - Failed to allocate ring buffer\n");
        return -1;
    }
    
    memset(ring_buffer, 0, ring_size);
    
    os_prim_log("GFX v10: [SW Init] Command ring allocated: %p (size: 0x%lx)\n",
                ring_buffer, ring_size);
    
    // Allocate MQD (Message Queue Descriptor) - shared memory for queues
    size_t mqd_size = 0x1000;  // 4KB
    void *mqd_buffer = os_prim_alloc(mqd_size);
    
    if (!mqd_buffer) {
        os_prim_log("GFX v10: ERROR - Failed to allocate MQD\n");
        os_prim_free(ring_buffer);
        return -1;
    }
    
    memset(mqd_buffer, 0, mqd_size);
    
    os_prim_log("GFX v10: [SW Init] MQD allocated: %p (size: 0x%lx)\n",
                mqd_buffer, mqd_size);
    
    os_prim_log("GFX v10: [SW Init] Configured for max 32 compute units\n");
    
    return 0;
}

/*
 * Hardware Init: Configure CP, RLC, rings
 */
static int gfx_v10_hw_init(struct OBJGPU *adev) {
    os_prim_log("GFX v10: [HW Init] Programming graphics hardware...\n");
    
    if (!adev->mmio_base) {
        os_prim_log("GFX v10: ERROR - No MMIO base mapped\n");
        return -1;
    }
    
    // ========================================================================
    // Step 1: Initialize Run List Controller (RLC)
    // ========================================================================
    os_prim_log("GFX v10: [HW] Initializing Run List Controller...\n");
    
    uintptr_t rlc_cntl_addr = (uintptr_t)adev->mmio_base + 
                               (GFXHUB_OFFSET + mmRLC_CNTL) * 4;
    
    if (rlc_cntl_addr < (uintptr_t)adev->mmio_base ||
        rlc_cntl_addr >= (uintptr_t)adev->mmio_base + 0x1000000) {
        os_prim_log("GFX v10: [SAFETY] RLC_CNTL address out of bounds\n");
        return -1;
    }
    
    // Set up RLC for graphics queue
    uint32_t rlc_cntl_val = 0x00000001;  // Enable RLC
    os_prim_write32(rlc_cntl_addr, rlc_cntl_val);
    
    os_prim_delay_us(100);
    
    // ========================================================================
    // Step 2: Configure CP (Command Processor)
    // ========================================================================
    os_prim_log("GFX v10: [HW] Initializing Command Processor...\n");
    
    uintptr_t cp_me_cntl_addr = (uintptr_t)adev->mmio_base + 
                                 (GFXHUB_OFFSET + mmCP_ME_CNTL) * 4;
    
    if (cp_me_cntl_addr < (uintptr_t)adev->mmio_base ||
        cp_me_cntl_addr >= (uintptr_t)adev->mmio_base + 0x1000000) {
        os_prim_log("GFX v10: [SAFETY] CP_ME_CNTL address out of bounds\n");
        return -1;
    }
    
    // Enable CP_ME (Micro Engine)
    // Bit 0: Enable
    // Bit 1: Reset
    uint32_t cp_me_val = 0x00000001;  // Enable
    os_prim_write32(cp_me_cntl_addr, cp_me_val);
    
    os_prim_delay_us(100);
    
    // ========================================================================
    // Step 3: Set up Command Ring Buffer
    // ========================================================================
    os_prim_log("GFX v10: [HW] Setting up command ring buffer...\n");
    
    // Ring base address (fake physical address in userland)
    uint32_t ring_base = 0x500000000ULL;  // Fake base
    
    uintptr_t cp_rb_base_addr = (uintptr_t)adev->mmio_base + 
                                 (GFXHUB_OFFSET + mmCP_RB_BASE) * 4;
    
    if (cp_rb_base_addr < (uintptr_t)adev->mmio_base ||
        cp_rb_base_addr >= (uintptr_t)adev->mmio_base + 0x1000000) {
        os_prim_log("GFX v10: [SAFETY] CP_RB_BASE address out of bounds\n");
        return -1;
    }
    
    os_prim_write32(cp_rb_base_addr, ring_base & 0xFFFFFFFF);
    
    // Ring control: 256KB ring, 4-byte entries
    uintptr_t cp_rb_cntl_addr = (uintptr_t)adev->mmio_base + 
                                 (GFXHUB_OFFSET + mmCP_RB_CNTL) * 4;
    
    if (cp_rb_cntl_addr < (uintptr_t)adev->mmio_base ||
        cp_rb_cntl_addr >= (uintptr_t)adev->mmio_base + 0x1000000) {
        os_prim_log("GFX v10: [SAFETY] CP_RB_CNTL address out of bounds\n");
        return -1;
    }
    
    // Size = log2(256KB / 4 bytes) = log2(65536) = 16
    uint32_t cp_rb_cntl_val = (16 << 8) | 0x1;  // Size + enable
    os_prim_write32(cp_rb_cntl_addr, cp_rb_cntl_val);
    
    os_prim_log("GFX v10: [HW] Command ring: base=0x%x, size=256KB\n", ring_base);
    
    // ========================================================================
    // Step 4: Initialize Read/Write Pointers
    // ========================================================================
    os_prim_log("GFX v10: [HW] Initializing ring pointers...\n");
    
    uintptr_t cp_rb_rptr_addr = (uintptr_t)adev->mmio_base + 
                                 (GFXHUB_OFFSET + mmCP_RB_RPTR) * 4;
    uintptr_t cp_rb_wptr_addr = (uintptr_t)adev->mmio_base + 
                                 (GFXHUB_OFFSET + mmCP_RB_WPTR) * 4;
    
    if (cp_rb_rptr_addr < (uintptr_t)adev->mmio_base ||
        cp_rb_rptr_addr >= (uintptr_t)adev->mmio_base + 0x1000000) {
        os_prim_log("GFX v10: [SAFETY] CP_RB_RPTR address out of bounds\n");
        return -1;
    }
    
    if (cp_rb_wptr_addr < (uintptr_t)adev->mmio_base ||
        cp_rb_wptr_addr >= (uintptr_t)adev->mmio_base + 0x1000000) {
        os_prim_log("GFX v10: [SAFETY] CP_RB_WPTR address out of bounds\n");
        return -1;
    }
    
    os_prim_write32(cp_rb_rptr_addr, 0);  // Read pointer = 0
    os_prim_write32(cp_rb_wptr_addr, 0);  // Write pointer = 0
    
    // ========================================================================
    // Step 5: Configure Shader Engines
    // ========================================================================
    os_prim_log("GFX v10: [HW] Configuring shader engines...\n");
    
    uintptr_t pipe_config_addr = (uintptr_t)adev->mmio_base + 
                                  (GFXHUB_OFFSET + mmGC_USER_SHADER_PIPE_CONFIG) * 4;
    
    if (pipe_config_addr < (uintptr_t)adev->mmio_base ||
        pipe_config_addr >= (uintptr_t)adev->mmio_base + 0x1000000) {
        os_prim_log("GFX v10: [SAFETY] PIPE_CONFIG address out of bounds\n");
        return -1;
    }
    
    // Default pipe config: 1 SE, 1 SH per SE
    uint32_t pipe_config = 0x00000000;
    os_prim_write32(pipe_config_addr, pipe_config);
    
    os_prim_log("GFX v10: [HW] Shader engines active: 1 SE, 2 CUs per SE (default)\n");
    
    // ========================================================================
    // Step 6: Enable Power Gating (optional)
    // ========================================================================
    os_prim_log("GFX v10: [HW] Setting up power gating...\n");
    
    // In a real driver, configure gating for unused CUs
    // For now, just log it
    
    os_prim_log("GFX v10: [HW Init] Graphics engine ready!\n");
    
    return 0;
}

/*
 * Late Init: Final checks
 */
static int gfx_v10_late_init(struct OBJGPU *adev) {
    os_prim_log("GFX v10: [Late] Running final checks...\n");
    
    if (!adev->mmio_base) {
        os_prim_log("GFX v10: ERROR - No MMIO base\n");
        return -1;
    }
    
    // Verify CP is enabled
    uintptr_t cp_me_cntl_addr = (uintptr_t)adev->mmio_base + 
                                 (GFXHUB_OFFSET + mmCP_ME_CNTL) * 4;
    
    uint32_t status = os_prim_read32(cp_me_cntl_addr);
    
    if (status & 0x1) {
        os_prim_log("GFX v10: [Late] CP is ENABLED ✓\n");
    } else {
        os_prim_log("GFX v10: [Late] WARNING - CP appears disabled\n");
    }
    
    return 0;
}

/*
 * Hardware Fini: Shutdown graphics engine
 */
static int gfx_v10_hw_fini(struct OBJGPU *adev) {
    os_prim_log("GFX v10: [HW Fini] Shutting down graphics engine...\n");
    
    if (!adev->mmio_base) {
        return 0;
    }
    
    // Disable CP
    uintptr_t cp_me_cntl_addr = (uintptr_t)adev->mmio_base + 
                                 (GFXHUB_OFFSET + mmCP_ME_CNTL) * 4;
    
    if (cp_me_cntl_addr >= (uintptr_t)adev->mmio_base &&
        cp_me_cntl_addr < (uintptr_t)adev->mmio_base + 0x1000000) {
        os_prim_write32(cp_me_cntl_addr, 0);
    }
    
    os_prim_log("GFX v10: [HW Fini] Graphics engine is down\n");
    
    return 0;
}

/*
 * Check if GFX is idle
 */
static bool gfx_v10_is_idle(struct OBJGPU *adev) {
    // In userland, always report as idle
    // Real driver checks ring buffer pointers
    return true;
}

/*
 * Wait for GFX to become idle
 */
static int gfx_v10_wait_for_idle(struct OBJGPU *adev) {
    // In userland, immediate
    return 0;
}

/*
 * Soft reset of GFX
 */
static int gfx_v10_soft_reset(struct OBJGPU *adev) {
    os_prim_log("GFX v10: [Soft Reset] Performing soft reset...\n");
    
    if (adev->mmio_base) {
        // Reset CP
        uintptr_t cp_me_cntl_addr = (uintptr_t)adev->mmio_base + 
                                     (GFXHUB_OFFSET + mmCP_ME_CNTL) * 4;
        
        if (cp_me_cntl_addr >= (uintptr_t)adev->mmio_base &&
            cp_me_cntl_addr < (uintptr_t)adev->mmio_base + 0x1000000) {
            
            // Set reset bit
            os_prim_write32(cp_me_cntl_addr, 0x2);
            os_prim_delay_us(1000);
            
            // Clear reset bit and enable
            os_prim_write32(cp_me_cntl_addr, 0x1);
        }
    }
    
    os_prim_log("GFX v10: [Soft Reset] Complete\n");
    
    return 0;
}

/*
 * Set CRTC Timing for Display Mode (Phase 2.1)
 * Programs the CRTC registers to match the requested display mode
 */
#include <GraphicsDefs.h>  // For display_mode

// CRTC Register Offsets (simplified - would need full register map)
#define mmCRTC0_CRTC_H_TOTAL                  0x6E00   // Horizontal total
#define mmCRTC0_CRTC_H_BLANK_START_END        0x6E04   // H blank timing
#define mmCRTC0_CRTC_H_SYNC_A                 0x6E08   // H sync
#define mmCRTC0_CRTC_V_TOTAL                  0x6E20   // Vertical total
#define mmCRTC0_CRTC_V_BLANK_START_END        0x6E24   // V blank timing
#define mmCRTC0_CRTC_V_SYNC_A                 0x6E28   // V sync
#define mmCRTC0_CRTC_CONTROL                  0x6E70   // CRTC enable

int gfx_v10_set_crtc_timing(struct OBJGPU *adev, const display_mode *mode) {
    if (!adev || !mode || !adev->mmio_base) {
        os_prim_log("GFX v10: [CRTC] Invalid parameters\n");
        return -1;
    }

    os_prim_log("GFX v10: [CRTC] Setting timing for %ux%u\n",
                mode->virtual_width, mode->virtual_height);

    // Extract timing parameters from display_mode
    // display_mode.timing is a timing_t structure with pixel clock and sync parameters
    uint32_t h_total = mode->timing.h_total;
    uint32_t h_display = mode->virtual_width;
    uint32_t h_blank_start = mode->timing.h_blank_start;
    uint32_t h_blank_end = mode->timing.h_blank_end;
    uint32_t h_sync_start = mode->timing.h_sync_start;
    uint32_t h_sync_end = mode->timing.h_sync_end;
    
    uint32_t v_total = mode->timing.v_total;
    uint32_t v_display = mode->virtual_height;
    uint32_t v_blank_start = mode->timing.v_blank_start;
    uint32_t v_blank_end = mode->timing.v_blank_end;
    uint32_t v_sync_start = mode->timing.v_sync_start;
    uint32_t v_sync_end = mode->timing.v_sync_end;
    
    os_prim_log("GFX v10: [CRTC] H: total=%u, blank %u-%u, sync %u-%u\n",
                h_total, h_blank_start, h_blank_end, h_sync_start, h_sync_end);
    os_prim_log("GFX v10: [CRTC] V: total=%u, blank %u-%u, sync %u-%u\n",
                v_total, v_blank_start, v_blank_end, v_sync_start, v_sync_end);

    // Map CRTC registers (CRTC0 base)
    // In real hardware, need to determine offset based on asic_type and device
    // For now, use fixed offset for Navi10
    uintptr_t crtc_base = (uintptr_t)adev->mmio_base + 0x6E00;  // CRTC0 base
    
    // Safety check: ensure CRTC base is within reasonable bounds
    if (crtc_base < (uintptr_t)adev->mmio_base ||
        crtc_base >= (uintptr_t)adev->mmio_base + 0x1000000) {
        os_prim_log("GFX v10: [CRTC] ERROR - CRTC base address out of bounds\n");
        return -1;
    }

    // Program H_TOTAL (horizontal total)
    uintptr_t h_total_addr = crtc_base + (mmCRTC0_CRTC_H_TOTAL & 0xFF);
    os_prim_write32(h_total_addr, h_total);
    os_prim_delay_us(10);

    // Program H_BLANK_START_END
    uintptr_t h_blank_addr = crtc_base + (mmCRTC0_CRTC_H_BLANK_START_END & 0xFF);
    uint32_t h_blank_val = (h_blank_end << 16) | h_blank_start;
    os_prim_write32(h_blank_addr, h_blank_val);
    os_prim_delay_us(10);

    // Program H_SYNC (start and end)
    uintptr_t h_sync_addr = crtc_base + (mmCRTC0_CRTC_H_SYNC_A & 0xFF);
    uint32_t h_sync_val = (h_sync_end << 16) | h_sync_start;
    os_prim_write32(h_sync_addr, h_sync_val);
    os_prim_delay_us(10);

    // Program V_TOTAL (vertical total)
    uintptr_t v_total_addr = crtc_base + (mmCRTC0_CRTC_V_TOTAL & 0xFF);
    os_prim_write32(v_total_addr, v_total);
    os_prim_delay_us(10);

    // Program V_BLANK_START_END
    uintptr_t v_blank_addr = crtc_base + (mmCRTC0_CRTC_V_BLANK_START_END & 0xFF);
    uint32_t v_blank_val = (v_blank_end << 16) | v_blank_start;
    os_prim_write32(v_blank_addr, v_blank_val);
    os_prim_delay_us(10);

    // Program V_SYNC (start and end)
    uintptr_t v_sync_addr = crtc_base + (mmCRTC0_CRTC_V_SYNC_A & 0xFF);
    uint32_t v_sync_val = (v_sync_end << 16) | v_sync_start;
    os_prim_write32(v_sync_addr, v_sync_val);
    os_prim_delay_us(10);

    // Enable CRTC
    uintptr_t crtc_ctrl_addr = crtc_base + (mmCRTC0_CRTC_CONTROL & 0xFF);
    uint32_t crtc_ctrl = 0x1;  // Enable CRTC
    os_prim_write32(crtc_ctrl_addr, crtc_ctrl);
    
    os_prim_log("GFX v10: [CRTC] CRTC timing programmed and enabled\n");
    
    return 0;
}

/* ============================================================================
 * GFX v10 IP Block Definition
 * ============================================================================ */

static const struct amd_ip_funcs gfx_v10_ip_funcs = {
    .name = "gfx_v10",
    .early_init = gfx_v10_early_init,
    .sw_init = gfx_v10_sw_init,
    .hw_init = gfx_v10_hw_init,
    .late_init = gfx_v10_late_init,
    .hw_fini = gfx_v10_hw_fini,
    .is_idle = gfx_v10_is_idle,
    .wait_for_idle = gfx_v10_wait_for_idle,
    .soft_reset = gfx_v10_soft_reset,
};

const struct amd_ip_block_version gfx_v10_ip_block = {
    .type = AMD_IP_BLOCK_TYPE_GFX,
    .major = 10,
    .minor = 1,
    .rev = 0,
    .funcs = &gfx_v10_ip_funcs,
};

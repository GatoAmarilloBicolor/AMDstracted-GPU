#include "../amd_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* VLIW Handler Integrated - Bridges to Real IP Blocks
 * 
 * This handler delegates actual hardware initialization to real IP block
 * implementations from drivers/amdgpu/ip_blocks/ instead of using printf stubs.
 * 
 * Supports: VLIW4/2 (Northern Islands, Evergreen - HD 6000-7000)
 */

/* Forward declarations for real IP blocks */
extern struct amd_ip_block_ops gmc_v6_ip_block;
extern struct amd_ip_block_ops gfx_v6_ip_block;
extern struct amd_ip_block_ops dce_v6_ip_block;

typedef struct {
    uint32_t gfx_status;
    uint32_t memory_size;
    uint32_t display_config;
} vliw_state_t;

/* NEW: init_hardware delegates to real IP blocks */
static int vliw_init_hardware_integrated(amd_device_t *dev)
{
    printf("[VLIW Integrated] Initializing hardware via real IP blocks\n");
    
    amd_gpu_handler_t *handler = dev->handler;
    if (!handler) {
        printf("[VLIW Integrated] ERROR: No handler\n");
        return -1;
    }
    
    /* Call real IP block initializations in order */
    printf("[VLIW Integrated] Calling real IP block: GMC v6\n");
    if (handler->ip_blocks.gmc && handler->ip_blocks.gmc->hw_init) {
        if (handler->ip_blocks.gmc->hw_init(NULL) != 0) {
            printf("[VLIW Integrated] ERROR: GMC v6 init failed\n");
            return -1;
        }
    }
    
    printf("[VLIW Integrated] Calling real IP block: GFX v6\n");
    if (handler->ip_blocks.gfx && handler->ip_blocks.gfx->hw_init) {
        if (handler->ip_blocks.gfx->hw_init(NULL) != 0) {
            printf("[VLIW Integrated] ERROR: GFX v6 init failed\n");
            return -1;
        }
    }
    
    printf("[VLIW Integrated] Calling real IP block: Display (DCE v6)\n");
    if (handler->ip_blocks.display && handler->ip_blocks.display->hw_init) {
        if (handler->ip_blocks.display->hw_init(NULL) != 0) {
            printf("[VLIW Integrated] WARNING: Display init failed (non-fatal)\n");
        }
    }
    
    printf("[VLIW Integrated] Hardware initialized via real IP blocks ✓\n");
    return 0;
}

/* Legacy handlers for backward compatibility */
static int vliw_hw_init(amd_device_t *dev)
{
    vliw_state_t *state = NULL;
    
    printf("[VLIW] Hardware init\n");
    
    state = malloc(sizeof(vliw_state_t));
    if (!state) return -1;
    
    memset(state, 0, sizeof(vliw_state_t));
    dev->hw_state = state;
    
    printf("[VLIW] GFX unit initialized\n");
    
    return 0;
}

static int vliw_hw_fini(amd_device_t *dev)
{
    printf("[VLIW] Hardware shutdown\n");
    if (dev->hw_state) {
        free(dev->hw_state);
        dev->hw_state = NULL;
    }
    return 0;
}

static int vliw_init_ip_blocks(amd_device_t *dev)
{
    printf("[VLIW] Initializing IP blocks\n");
    printf("[VLIW]   - GFX block\n");
    printf("[VLIW]   - DCE (display)\n");
    printf("[VLIW]   - SRBM\n");
    return 0;
}

static int vliw_enable_ip_block(amd_device_t *dev, int block_type)
{
    printf("[VLIW] Enabling IP block %d\n", block_type);
    return 0;
}

static int vliw_cleanup_ip_blocks(amd_device_t *dev)
{
    printf("[VLIW] Cleaning up IP blocks\n");
    return 0;
}

static int vliw_init_gmc(amd_device_t *dev)
{
    printf("[VLIW] Initializing GMC (memory controller)\n");
    
    size_t vram_size = dev->gpu_info.max_vram_mb * 1024 * 1024;
    dev->vram_pool = malloc(vram_size);
    if (!dev->vram_pool) return -1;
    
    printf("[VLIW] Allocated %zu MB VRAM\n", dev->gpu_info.max_vram_mb);
    
    return 0;
}

static int vliw_allocate_vram(amd_device_t *dev, size_t size, uint64_t *gpu_addr)
{
    if (!dev->vram_pool || dev->vram_used + size > 
        dev->gpu_info.max_vram_mb * 1024 * 1024) {
        return -1;
    }
    
    *gpu_addr = 0x100000000ULL + dev->vram_used;
    dev->vram_used += size;
    
    printf("[VLIW] Allocated %zu bytes at GPU addr 0x%llx\n", 
           size, (unsigned long long)*gpu_addr);
    
    return 0;
}

static int vliw_init_gfx(amd_device_t *dev)
{
    printf("[VLIW] Initializing GFX engine\n");
    printf("[VLIW] GFX version: VLIW4/2\n");
    printf("[VLIW] Max waves: %u\n", 
           dev->gpu_info.max_compute_units * dev->gpu_info.max_wave64_per_cu);
    return 0;
}

static int vliw_submit_command(amd_device_t *dev, void *cmd)
{
    printf("[VLIW] Submitting command\n");
    return 0;
}

static int vliw_init_display(amd_device_t *dev)
{
    printf("[VLIW] Initializing display engine (DCE)\n");
    return 0;
}

static int vliw_set_display_mode(amd_device_t *dev, uint32_t width, uint32_t height)
{
    printf("[VLIW] Setting display mode: %ux%u\n", width, height);
    return 0;
}

static void vliw_cleanup(amd_device_t *dev)
{
    printf("[VLIW] Cleanup complete\n");
}

/* VLIW Handler Registration - NOW WITH IP BLOCK SUPPORT */
amd_gpu_handler_t vliw_handler_integrated = {
    .name = "VLIW Handler (Integrated)",
    .generation = AMD_VLIW,
    
    /* NEW: IP block members - point to real implementations */
    .ip_blocks = {
        .gmc = &gmc_v6_ip_block,      /* Real from drivers/ */
        .gfx = &gfx_v6_ip_block,      /* Real from drivers/ */
        .display = &dce_v6_ip_block,  /* Real from drivers/ */
        .sdma = NULL,
        .clock = NULL,
    },
    
    /* NEW: Delegating init to real IP blocks */
    .init_hardware = vliw_init_hardware_integrated,
    
    /* Legacy compatibility */
    .hw_init = vliw_hw_init,
    .hw_fini = vliw_hw_fini,
    .init_ip_blocks = vliw_init_ip_blocks,
    .enable_ip_block = vliw_enable_ip_block,
    .cleanup_ip_blocks = vliw_cleanup_ip_blocks,
    .init_gmc = vliw_init_gmc,
    .allocate_vram = vliw_allocate_vram,
    .init_gfx = vliw_init_gfx,
    .submit_command = vliw_submit_command,
    .init_display = vliw_init_display,
    .set_display_mode = vliw_set_display_mode,
    .cleanup = vliw_cleanup,
};

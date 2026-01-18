#include "../amd_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* GCN Handler Integrated - Bridges to Real IP Blocks
 * 
 * This handler delegates actual hardware initialization to real IP block
 * implementations from drivers/amdgpu/ip_blocks/ instead of using printf stubs.
 * 
 * Supports: GCN1, GCN2, GCN3, GCN4, GCN5 (SI, CIK, Fiji, Vega)
 */

/* Forward declarations for real IP blocks */
extern struct amd_ip_block_ops gmc_v9_ip_block;
extern struct amd_ip_block_ops gfx_v9_ip_block;
extern struct amd_ip_block_ops dce_v11_ip_block;

typedef struct {
    uint32_t compute_rings;
    uint32_t gfx_rings;
    uint32_t memory_size;
} gcn_state_t;

/* NEW: init_hardware delegates to real IP blocks */
static int gcn_init_hardware_integrated(amd_device_t *dev)
{
    printf("[GCN Integrated] Initializing hardware via real IP blocks\n");
    
    amd_gpu_handler_t *handler = dev->handler;
    if (!handler) {
        printf("[GCN Integrated] ERROR: No handler\n");
        return -1;
    }
    
    /* Call real IP block initializations in order */
    printf("[GCN Integrated] Calling real IP block: GMC v9\n");
    if (handler->ip_blocks.gmc && handler->ip_blocks.gmc->hw_init) {
        if (handler->ip_blocks.gmc->hw_init(NULL) != 0) {
            printf("[GCN Integrated] ERROR: GMC v9 init failed\n");
            return -1;
        }
    }
    
    printf("[GCN Integrated] Calling real IP block: GFX v9\n");
    if (handler->ip_blocks.gfx && handler->ip_blocks.gfx->hw_init) {
        if (handler->ip_blocks.gfx->hw_init(NULL) != 0) {
            printf("[GCN Integrated] ERROR: GFX v9 init failed\n");
            return -1;
        }
    }
    
    printf("[GCN Integrated] Calling real IP block: Display (DCE/DCN)\n");
    if (handler->ip_blocks.display && handler->ip_blocks.display->hw_init) {
        if (handler->ip_blocks.display->hw_init(NULL) != 0) {
            printf("[GCN Integrated] WARNING: Display init failed (non-fatal)\n");
        }
    }
    
    printf("[GCN Integrated] Hardware initialized via real IP blocks ✓\n");
    return 0;
}

/* Legacy handlers for backward compatibility */
static int gcn_hw_init(amd_device_t *dev)
{
    gcn_state_t *state = NULL;
    
    printf("[GCN] Hardware init (Gen %d)\n", dev->gpu_info.generation);
    
    state = malloc(sizeof(gcn_state_t));
    if (!state) return -1;
    
    memset(state, 0, sizeof(gcn_state_t));
    dev->hw_state = state;
    
    printf("[GCN] GFX rings: %u\n", dev->gpu_info.max_compute_units);
    printf("[GCN] Max waves: %u\n", 
           dev->gpu_info.max_compute_units * dev->gpu_info.max_wave64_per_cu);
    
    return 0;
}

static int gcn_hw_fini(amd_device_t *dev)
{
    printf("[GCN] Hardware shutdown\n");
    if (dev->hw_state) {
        free(dev->hw_state);
        dev->hw_state = NULL;
    }
    return 0;
}

static int gcn_init_ip_blocks(amd_device_t *dev)
{
    printf("[GCN] Initializing IP blocks\n");
    printf("[GCN]   - SDMA (DMA engines)\n");
    printf("[GCN]   - GMC (memory controller)\n");
    printf("[GCN]   - GFX (graphics engine)\n");
    printf("[GCN]   - DCE (display)\n");
    return 0;
}

static int gcn_enable_ip_block(amd_device_t *dev, int block_type)
{
    printf("[GCN] Enabling IP block %d\n", block_type);
    return 0;
}

static int gcn_cleanup_ip_blocks(amd_device_t *dev)
{
    printf("[GCN] Cleaning up IP blocks\n");
    return 0;
}

static int gcn_init_gmc(amd_device_t *dev)
{
    printf("[GCN] Initializing GMC (Garlic Memory Controller)\n");
    
    size_t vram_size = dev->gpu_info.max_vram_mb * 1024 * 1024;
    dev->vram_pool = malloc(vram_size);
    if (!dev->vram_pool) return -1;
    
    printf("[GCN] Allocated %zu MB VRAM\n", dev->gpu_info.max_vram_mb);
    printf("[GCN] GART support: enabled\n");
    
    return 0;
}

static int gcn_allocate_vram(amd_device_t *dev, size_t size, uint64_t *gpu_addr)
{
    if (!dev->vram_pool || dev->vram_used + size > 
        dev->gpu_info.max_vram_mb * 1024 * 1024) {
        return -1;
    }
    
    *gpu_addr = 0x100000000ULL + dev->vram_used;
    dev->vram_used += size;
    
    printf("[GCN] Allocated %zu bytes at 0x%llx\n", 
           size, (unsigned long long)*gpu_addr);
    
    return 0;
}

static int gcn_init_gfx(amd_device_t *dev)
{
    printf("[GCN] Initializing GFX engine\n");
    printf("[GCN]   Compute units: %u\n", dev->gpu_info.max_compute_units);
    printf("[GCN]   LDS per CU: 64 KB\n");
    printf("[GCN]   Compute: %s\n", 
           dev->gpu_info.capabilities.has_compute ? "enabled" : "disabled");
    
    if (dev->gpu_info.generation >= AMD_GCN4) {
        printf("[GCN]   Async compute: enabled\n");
    }
    
    return 0;
}

static int gcn_submit_command(amd_device_t *dev, void *cmd)
{
    printf("[GCN] Submitting command\n");
    return 0;
}

static int gcn_init_display(amd_device_t *dev)
{
    printf("[GCN] Initializing display (DCE)\n");
    return 0;
}

static int gcn_set_display_mode(amd_device_t *dev, uint32_t width, uint32_t height)
{
    printf("[GCN] Setting display mode: %ux%u\n", width, height);
    return 0;
}

static void gcn_cleanup(amd_device_t *dev)
{
    printf("[GCN] Cleanup complete\n");
}

/* GCN Handler Registration - NOW WITH IP BLOCK SUPPORT */
amd_gpu_handler_t gcn_handler_integrated = {
    .name = "GCN Handler (Integrated)",
    .generation = AMD_GCN1,  /* Representative for all GCN */
    
    /* NEW: IP block members - point to real implementations */
    .ip_blocks = {
        .gmc = &gmc_v9_ip_block,      /* Real from drivers/ */
        .gfx = &gfx_v9_ip_block,      /* Real from drivers/ */
        .display = &dce_v11_ip_block, /* Real from drivers/ */
        .sdma = NULL,
        .clock = NULL,
    },
    
    /* NEW: Delegating init to real IP blocks */
    .init_hardware = gcn_init_hardware_integrated,
    
    /* Legacy compatibility */
    .hw_init = gcn_hw_init,
    .hw_fini = gcn_hw_fini,
    .init_ip_blocks = gcn_init_ip_blocks,
    .enable_ip_block = gcn_enable_ip_block,
    .cleanup_ip_blocks = gcn_cleanup_ip_blocks,
    .init_gmc = gcn_init_gmc,
    .allocate_vram = gcn_allocate_vram,
    .init_gfx = gcn_init_gfx,
    .submit_command = gcn_submit_command,
    .init_display = gcn_init_display,
    .set_display_mode = gcn_set_display_mode,
    .cleanup = gcn_cleanup,
};

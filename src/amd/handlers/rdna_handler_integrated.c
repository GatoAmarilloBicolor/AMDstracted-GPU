#include "../amd_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* RDNA Handler Integrated - Bridges to Real IP Blocks
 * 
 * This handler delegates actual hardware initialization to real IP block
 * implementations from drivers/amdgpu/ip_blocks/ instead of using printf stubs.
 * 
 * This implements the Redesign Architecture Proposal pattern where handlers
 * contain IP block members and call real implementations.
 */

/* Forward declarations for real IP blocks */
extern struct amd_ip_block_ops gmc_v10_ip_block;
extern struct amd_ip_block_ops gfx_v10_ip_block;
extern struct amd_ip_block_ops dce_v10_ip_block;
extern struct amd_ip_block_ops dcn_v1_ip_block;

typedef struct {
    uint32_t compute_rings;
    uint32_t gfx_rings;
    uint32_t memory_size;
    bool raytracing_enabled;
} rdna_state_t;

/* NEW: init_hardware delegates to real IP blocks */
static int rdna_init_hardware_integrated(amd_device_t *dev)
{
    printf("[RDNA Integrated] Initializing hardware via real IP blocks\n");
    
    /* Get the handler's IP block pointers */
    amd_gpu_handler_t *handler = dev->handler;
    if (!handler) {
        printf("[RDNA Integrated] ERROR: No handler\n");
        return -1;
    }
    
    /* Call real IP block initializations in order */
    printf("[RDNA Integrated] Calling real IP block: GMC v10\n");
    if (handler->ip_blocks.gmc && handler->ip_blocks.gmc->hw_init) {
        if (handler->ip_blocks.gmc->hw_init(NULL) != 0) {
            printf("[RDNA Integrated] ERROR: GMC v10 init failed\n");
            return -1;
        }
    }
    
    printf("[RDNA Integrated] Calling real IP block: GFX v10\n");
    if (handler->ip_blocks.gfx && handler->ip_blocks.gfx->hw_init) {
        if (handler->ip_blocks.gfx->hw_init(NULL) != 0) {
            printf("[RDNA Integrated] ERROR: GFX v10 init failed\n");
            return -1;
        }
    }
    
    printf("[RDNA Integrated] Calling real IP block: Display\n");
    if (handler->ip_blocks.display && handler->ip_blocks.display->hw_init) {
        if (handler->ip_blocks.display->hw_init(NULL) != 0) {
            printf("[RDNA Integrated] WARNING: Display init failed (non-fatal)\n");
        }
    }
    
    printf("[RDNA Integrated] Hardware initialized via real IP blocks ✓\n");
    return 0;
}

/* Legacy handlers for backward compatibility */
static int rdna_hw_init(amd_device_t *dev)
{
    rdna_state_t *state = NULL;
    
    printf("[RDNA] Hardware init (Gen %d)\n", dev->gpu_info.generation);
    
    state = malloc(sizeof(rdna_state_t));
    if (!state) return -1;
    
    memset(state, 0, sizeof(rdna_state_t));
    dev->hw_state = state;
    
    printf("[RDNA] RDNA architecture detected\n");
    printf("[RDNA] Max CUs: %u\n", dev->gpu_info.max_compute_units);
    printf("[RDNA] Max waves: %u\n",
           dev->gpu_info.max_compute_units * dev->gpu_info.max_wave64_per_cu);
    
    if (dev->gpu_info.capabilities.has_raytracing) {
        printf("[RDNA] Ray tracing: enabled\n");
        state->raytracing_enabled = true;
    }
    
    return 0;
}

static int rdna_hw_fini(amd_device_t *dev)
{
    printf("[RDNA] Hardware shutdown\n");
    if (dev->hw_state) {
        free(dev->hw_state);
        dev->hw_state = NULL;
    }
    return 0;
}

static int rdna_init_ip_blocks(amd_device_t *dev)
{
    printf("[RDNA] Initializing IP blocks\n");
    printf("[RDNA]   - SDMA (DMA engines)\n");
    printf("[RDNA]   - HUB (memory hub)\n");
    printf("[RDNA]   - SQC (scalar cache)\n");
    printf("[RDNA]   - GFX (graphics engine)\n");
    printf("[RDNA]   - DCN (display core next)\n");
    
    if (dev->gpu_info.capabilities.has_raytracing) {
        printf("[RDNA]   - RT core (ray tracing accelerators)\n");
    }
    
    return 0;
}

static int rdna_enable_ip_block(amd_device_t *dev, int block_type)
{
    printf("[RDNA] Enabling IP block %d\n", block_type);
    return 0;
}

static int rdna_cleanup_ip_blocks(amd_device_t *dev)
{
    printf("[RDNA] Cleaning up IP blocks\n");
    return 0;
}

static int rdna_init_gmc(amd_device_t *dev)
{
    printf("[RDNA] Initializing HUB (memory controller)\n");
    
    size_t vram_size = dev->gpu_info.max_vram_mb * 1024 * 1024;
    dev->vram_pool = malloc(vram_size);
    if (!dev->vram_pool) return -1;
    
    printf("[RDNA] Allocated %zu MB VRAM\n", dev->gpu_info.max_vram_mb);
    printf("[RDNA] GART support: enabled\n");
    printf("[RDNA] Virtual addressing: enabled\n");
    
    return 0;
}

static int rdna_allocate_vram(amd_device_t *dev, size_t size, uint64_t *gpu_addr)
{
    if (!dev->vram_pool || dev->vram_used + size > 
        dev->gpu_info.max_vram_mb * 1024 * 1024) {
        return -1;
    }
    
    *gpu_addr = 0x100000000ULL + dev->vram_used;
    dev->vram_used += size;
    
    printf("[RDNA] Allocated %zu bytes at 0x%llx\n", 
           size, (unsigned long long)*gpu_addr);
    
    return 0;
}

static int rdna_init_gfx(amd_device_t *dev)
{
    printf("[RDNA] Initializing GFX engine\n");
    printf("[RDNA]   Compute units: %u\n", dev->gpu_info.max_compute_units);
    printf("[RDNA]   LDS per CU: 128 KB\n");
    printf("[RDNA]   Compute: enabled\n");
    printf("[RDNA]   Async compute: enabled\n");
    printf("[RDNA]   Preemption: enabled\n");
    
    return 0;
}

static int rdna_submit_command(amd_device_t *dev, void *cmd)
{
    printf("[RDNA] Submitting command\n");
    return 0;
}

static int rdna_init_display(amd_device_t *dev)
{
    printf("[RDNA] Initializing display (DCN)\n");
    printf("[RDNA] Display Core Next (DCN) - Modern display engine\n");
    return 0;
}

static int rdna_set_display_mode(amd_device_t *dev, uint32_t width, uint32_t height)
{
    printf("[RDNA] Setting display mode: %ux%u\n", width, height);
    return 0;
}

static void rdna_cleanup(amd_device_t *dev)
{
    printf("[RDNA] Cleanup complete\n");
}

/* RDNA Handler Registration - NOW WITH IP BLOCK SUPPORT */
amd_gpu_handler_t rdna_handler_integrated = {
    .name = "RDNA Handler (Integrated)",
    .generation = AMD_RDNA2,
    
    /* NEW: IP block members - populated at init time */
    .ip_blocks = {
        .gmc = &gmc_v10_ip_block,
        .gfx = &gfx_v10_ip_block,
        .display = &dcn_v1_ip_block,
        .sdma = NULL,
        .clock = NULL,
    },
    
    /* NEW: Delegating init to real IP blocks */
    .init_hardware = rdna_init_hardware_integrated,
    
    /* Legacy compatibility */
    .hw_init = rdna_hw_init,
    .hw_fini = rdna_hw_fini,
    .init_ip_blocks = rdna_init_ip_blocks,
    .enable_ip_block = rdna_enable_ip_block,
    .cleanup_ip_blocks = rdna_cleanup_ip_blocks,
    .init_gmc = rdna_init_gmc,
    .allocate_vram = rdna_allocate_vram,
    .init_gfx = rdna_init_gfx,
    .submit_command = rdna_submit_command,
    .init_display = rdna_init_display,
    .set_display_mode = rdna_set_display_mode,
    .cleanup = rdna_cleanup,
};

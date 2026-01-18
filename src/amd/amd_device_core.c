#include "amd_device.h"
#include "amd_backend_detection.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Forward declarations for handlers */
extern amd_gpu_handler_t vliw_handler;
extern amd_gpu_handler_t gcn_handler;
extern amd_gpu_handler_t rdna_handler;

/* Get appropriate handler for GPU generation */
amd_gpu_handler_t* amd_get_handler(amd_gpu_generation_t generation)
{
    switch (generation) {
        case AMD_VLIW:
            return &vliw_handler;
        case AMD_GCN1:
        case AMD_GCN2:
        case AMD_GCN3:
        case AMD_GCN4:
        case AMD_GCN5:
            return &gcn_handler;
        case AMD_RDNA2:
        case AMD_RDNA3:
            return &rdna_handler;
        default:
            return NULL;
    }
}

/* Device Allocation */
static amd_device_t* amd_device_alloc(void)
{
    amd_device_t *dev = malloc(sizeof(amd_device_t));
    if (!dev) return NULL;
    
    memset(dev, 0, sizeof(amd_device_t));
    dev->ref_count = 1;
    dev->vram_used = 0;
    
    return dev;
}

/* Device Probe: Find GPU by device ID */
int amd_device_probe(uint16_t device_id, amd_device_t **dev_out)
{
    amd_device_t *dev = NULL;
    amd_gpu_device_info_t *gpu_info = NULL;
    amd_gpu_handler_t *handler = NULL;
    
    if (!dev_out) return -1;
    
    /* Lookup device in database */
    gpu_info = amd_device_lookup(device_id);
    if (!gpu_info) {
        printf("ERROR: Unknown AMD device [1002:%04x]\n", device_id);
        return -1;
    }
    
    /* Allocate device structure */
    dev = amd_device_alloc();
    if (!dev) return -1;
    
    /* Copy GPU info */
    memcpy(&dev->gpu_info, gpu_info, sizeof(amd_gpu_device_info_t));
    
    /* Get generation-specific handler */
    handler = amd_get_handler(gpu_info->generation);
    if (!handler) {
        printf("ERROR: No handler for generation %d\n", gpu_info->generation);
        free(dev);
        return -1;
    }
    
    dev->handler = handler;
    
    /* Select backend (can be overridden by module parameters) */
    dev->gpu_info.preferred_backend = amd_select_backend(&dev->gpu_info);
    
    *dev_out = dev;
    printf("Probed: %s [%04x:%04x] - Gen %d\n",
           gpu_info->codename, gpu_info->vendor_id, gpu_info->device_id,
           gpu_info->generation);
    
    return 0;
}

/* Device Initialization with Backend Detection */
int amd_device_init(amd_device_t *dev)
{
    int ret = 0;
    amd_backend_support_t backend_support;
    
    if (!dev || !dev->handler) return -1;
    
    printf("Initializing %s...\n", dev->gpu_info.codename);
    
    /* Detect backend support on this system */
    printf("\nDetecting backend support on this system:\n");
    if (amd_detect_backend_support(&backend_support) == 0) {
        amd_setup_backend_env(&backend_support);
        amd_print_backend_support(&backend_support);
        
        /* Override hardware preference if system doesn't support it */
        if (!backend_support.system_has_radv && 
            dev->gpu_info.preferred_backend == AMD_BACKEND_RADV) {
            printf("NOTE: RADV not available, falling back to Mesa\n");
            dev->gpu_info.preferred_backend = AMD_BACKEND_MESA;
        }
    }
    printf("\n");
    
    /* Hardware initialization */
    if (dev->handler->hw_init) {
        ret = dev->handler->hw_init(dev);
        if (ret < 0) {
            printf("ERROR: Hardware init failed\n");
            return ret;
        }
    }
    
    /* IP block initialization */
    if (dev->handler->init_ip_blocks) {
        ret = dev->handler->init_ip_blocks(dev);
        if (ret < 0) {
            printf("ERROR: IP block init failed\n");
            if (dev->handler->hw_fini)
                dev->handler->hw_fini(dev);
            return ret;
        }
    }
    
    /* Memory management initialization */
    if (dev->handler->init_gmc) {
        ret = dev->handler->init_gmc(dev);
        if (ret < 0) {
            printf("ERROR: GMC init failed\n");
            if (dev->handler->cleanup_ip_blocks)
                dev->handler->cleanup_ip_blocks(dev);
            if (dev->handler->hw_fini)
                dev->handler->hw_fini(dev);
            return ret;
        }
    }
    
    /* Graphics engine initialization */
    if (dev->handler->init_gfx) {
        ret = dev->handler->init_gfx(dev);
        if (ret < 0) {
            printf("ERROR: GFX init failed\n");
            if (dev->handler->cleanup_ip_blocks)
                dev->handler->cleanup_ip_blocks(dev);
            if (dev->handler->hw_fini)
                dev->handler->hw_fini(dev);
            return ret;
        }
    }
    
    /* Display initialization */
    if (dev->handler->init_display) {
        ret = dev->handler->init_display(dev);
        if (ret < 0) {
            printf("WARNING: Display init failed (non-fatal)\n");
        }
    }
    
    dev->initialized = true;
    printf("Successfully initialized %s with %s backend\n",
           dev->gpu_info.codename,
           dev->gpu_info.preferred_backend == AMD_BACKEND_RADV ? "RADV" :
           dev->gpu_info.preferred_backend == AMD_BACKEND_MESA ? "Mesa" :
           "Software");
    
    return 0;
}

/* Device Finalization */
int amd_device_fini(amd_device_t *dev)
{
    if (!dev || !dev->handler) return -1;
    
    if (!dev->initialized) return 0;
    
    printf("Shutting down %s...\n", dev->gpu_info.codename);
    
    /* Cleanup in reverse order */
    if (dev->handler->cleanup_ip_blocks) {
        dev->handler->cleanup_ip_blocks(dev);
    }
    
    if (dev->handler->hw_fini) {
        dev->handler->hw_fini(dev);
    }
    
    if (dev->handler->cleanup) {
        dev->handler->cleanup(dev);
    }
    
    dev->initialized = false;
    printf("Device shutdown complete\n");
    
    return 0;
}

/* Device Deallocation */
void amd_device_free(amd_device_t *dev)
{
    if (!dev) return;
    
    if (dev->initialized)
        amd_device_fini(dev);
    
    if (dev->vram_pool)
        free(dev->vram_pool);
    
    if (dev->hw_state)
        free(dev->hw_state);
    
    free(dev);
}

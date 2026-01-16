// RADV RMAPI Winsys - Vulkan backend for AMD GPU via RMAPI IPC
// This replaces DRM ioctl with RMAPI calls for OS-agnostic Vulkan

#include <stddef.h>
#include <stdint.h>
#include "../../../../src/amd/rmapi.h"  // RMAPI functions

// Stub implementations - replace DRM calls with RMAPI

struct radv_rmapi_winsys {
    // RMAPI connection
};

static int radv_rmapi_winsys_alloc_memory(struct radv_rmapi_winsys *ws, size_t size, void **ptr) {
    uint64_t addr;
    int ret = rmapi_alloc_memory(NULL, size, &addr);
    *ptr = (void*)addr;
    return ret;
}

static void radv_rmapi_winsys_free_memory(struct radv_rmapi_winsys *ws, void *ptr) {
    rmapi_free_memory(NULL, (uint64_t)ptr);
}

// Add more functions as needed, replacing DRM with RMAPI

// This allows RADV to work without kernel DRM, using RMAPI IPC for hardware access.
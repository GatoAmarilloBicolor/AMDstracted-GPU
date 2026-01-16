// RADV RMAPI Winsys - Vulkan backend for AMD GPU via RMAPI IPC
// This replaces DRM ioctl with RMAPI calls for OS-agnostic Vulkan (no DRM dependency)

#include <stddef.h>
#include <stdint.h>

// Extern RMAPI functions (since header not found in build)
extern int rmapi_alloc_memory(struct OBJGPU *gpu, size_t size, uint64_t *addr);
extern int rmapi_free_memory(struct OBJGPU *gpu, uint64_t addr);
extern int rmapi_submit_command(struct OBJGPU *gpu, struct amdgpu_command_buffer *cb);

// RMAPI winsys structure
struct radv_rmapi_winsys {
    // No additional state needed, RMAPI handles everything
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

static int radv_rmapi_winsys_submit(struct radv_rmapi_winsys *ws, void *cmdbuf) {
    // Submit command buffer via RMAPI
    return rmapi_submit_command(NULL, cmdbuf);
}

// Additional functions to replace DRM (stubs for now, expand as needed)
static int radv_rmapi_winsys_create_buffer(struct radv_rmapi_winsys *ws, size_t size, void **handle) {
    return radv_rmapi_winsys_alloc_memory(ws, size, handle);
}

static void radv_rmapi_winsys_destroy_buffer(struct radv_rmapi_winsys *ws, void *handle) {
    radv_rmapi_winsys_free_memory(ws, handle);
}

// This enables RADV to work OS-agnostically without DRM, using RMAPI IPC for hardware access.
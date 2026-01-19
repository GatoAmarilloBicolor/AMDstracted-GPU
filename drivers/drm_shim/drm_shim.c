/*
 * DRM Shim Library
 *
 * Intercepts libdrm_amdgpu calls and translates to RMAPI
 */

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "drm_to_rmapi.h"
#include "device_manager.h"

// Stub types for libdrm_amdgpu
typedef void *amdgpu_device_handle;
typedef void *amdgpu_context_handle;

struct amdgpu_bo_alloc_request {
    uint64_t alloc_size;
    // Other fields stubbed
};

struct drm_amdgpu_cs_chunk {
    uint32_t chunk_id;
    uint32_t length_dw;
    uint64_t chunk_data;
};

#define AMDGPU_CHUNK_ID_IB 0x01

struct drm_amdgpu_cs_chunk_ib {
    uint64_t ib_data;
    uint32_t ib_bytes;
    // Other fields stubbed
};

struct drm_amdgpu_info {
    // Stub structure
    uint32_t device_id;
};

// Original libdrm_amdgpu functions
static void *orig_lib = NULL;

// Function pointers to original functions
static int (*orig_amdgpu_device_initialize)(int fd, uint32_t *major, uint32_t *minor, amdgpu_device_handle *dev) = NULL;

__attribute__((constructor))
static void init_shim(void)
{
    printf("[DRM Shim] Initializing...\n");
    device_manager_init();

    // Load original libdrm_amdgpu
    orig_lib = dlopen("libdrm_amdgpu.so.1", RTLD_LAZY);
    if (!orig_lib) {
        fprintf(stderr, "[DRM Shim] Failed to load original libdrm_amdgpu: %s\n", dlerror());
    }

    // Get function pointers
    orig_amdgpu_device_initialize = dlsym(orig_lib, "amdgpu_device_initialize");
}

__attribute__((destructor))
static void fini_shim(void)
{
    printf("[DRM Shim] Finalizing...\n");
    device_manager_fini();
    if (orig_lib) {
        dlclose(orig_lib);
    }
}

// Shimmed functions

int amdgpu_device_initialize(int fd, uint32_t *major, uint32_t *minor, amdgpu_device_handle *dev)
{
    printf("[DRM Shim] amdgpu_device_initialize(fd=%d)\n", fd);

    // Get or create RMAPI device
    rmapi_device *rmapi_dev = device_manager_get(fd);
    if (!rmapi_dev) {
        return -1;
    }

    // TODO: Initialize device properly
    // For now, set major/minor
    if (major) *major = 1;
    if (minor) *minor = 0;
    if (dev) *dev = (amdgpu_device_handle)fd;  // Use fd as handle

    return 0;
}

int amdgpu_device_deinitialize(amdgpu_device_handle dev)
{
    printf("[DRM Shim] amdgpu_device_deinitialize(dev=%p)\n", dev);
    int fd = (int)dev;
    return device_manager_remove(fd);
}

int amdgpu_bo_alloc(amdgpu_device_handle dev, struct amdgpu_bo_alloc_request *alloc_buffer, uint32_t *buf_handle)
{
    printf("[DRM Shim] amdgpu_bo_alloc(dev=%p, size=%llu)\n", dev, alloc_buffer->alloc_size);
    int fd = (int)dev;
    uint64_t va;
    return drm_alloc_to_rmapi(fd, alloc_buffer->alloc_size, buf_handle, &va);
}

int amdgpu_bo_free(amdgpu_device_handle dev, uint32_t buf_handle)
{
    printf("[DRM Shim] amdgpu_bo_free(dev=%p, handle=%u)\n", dev, buf_handle);
    int fd = (int)dev;
    return drm_free_to_rmapi(fd, buf_handle);
}

int amdgpu_bo_cpu_map(amdgpu_device_handle dev, uint32_t buf_handle, void **cpu)
{
    printf("[DRM Shim] amdgpu_bo_cpu_map(dev=%p, handle=%u)\n", dev, buf_handle);
    int fd = (int)dev;
    return drm_map_to_rmapi(fd, buf_handle, 0, 0, cpu);  // offset=0, size=0 for full map
}

int amdgpu_bo_cpu_unmap(amdgpu_device_handle dev, uint32_t buf_handle)
{
    printf("[DRM Shim] amdgpu_bo_cpu_unmap(dev=%p, handle=%u)\n", dev, buf_handle);
    int fd = (int)dev;
    return drm_unmap_to_rmapi(fd, buf_handle, NULL);
}

int amdgpu_cs_submit(amdgpu_context_handle ctx, uint32_t bo_list_handle, uint32_t num_chunks, struct drm_amdgpu_cs_chunk *chunks, uint64_t *seq_no)
{
    printf("[DRM Shim] amdgpu_cs_submit(ctx=%p, num_chunks=%u)\n", ctx, num_chunks);

    // TODO: Extract command buffer from chunks
    // For stub, assume first chunk is IB
    if (num_chunks > 0 && chunks[0].chunk_id == AMDGPU_CHUNK_ID_IB) {
        struct drm_amdgpu_cs_chunk_ib *ib = (struct drm_amdgpu_cs_chunk_ib *)chunks[0].chunk_data;
        void *cmd_buffer = (void *)ib->ib_data;  // Assume offset
        uint32_t cmd_size = ib->ib_bytes / 4;  // Assume dwords

        int fd = 0;  // TODO: get fd from ctx
        return drm_cs_submit_to_rmapi(fd, cmd_buffer, cmd_size, 0);
    }

    return 0;
}

int amdgpu_cs_wait_fences(amdgpu_context_handle ctx, uint32_t fence_count, uint32_t *handles, bool wait_all, uint64_t timeout_ns, uint32_t *status, uint64_t *first)
{
    printf("[DRM Shim] amdgpu_cs_wait_fences(ctx=%p, fence_count=%u)\n", ctx, fence_count);

    int fd = 0;  // TODO: get fd from ctx
    return drm_cs_wait_to_rmapi(fd, timeout_ns);
}

int amdgpu_query_gpu_info(amdgpu_device_handle dev, struct drm_amdgpu_info *info)
{
    printf("[DRM Shim] amdgpu_query_gpu_info(dev=%p)\n", dev);
    int fd = (int)dev;
    return drm_query_gpu_info_to_rmapi(fd, info, sizeof(*info));
}

// Add more functions as needed...
/*
 * DRM Radeon Shim Library
 *
 * Intercepts libdrm_radeon calls and translates to RMAPI
 * For legacy R600/R300 GPUs that use radeon API instead of amdgpu
 */

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "drm_to_rmapi.h"
#include "device_manager.h"

// Original libdrm_radeon functions
static void *orig_lib = NULL;

// Function pointers to original functions (if needed)
static int (*orig_radeon_device_initialize)(int fd, uint32_t *major, uint32_t *minor, struct radeon_device **dev) = NULL;

__attribute__((constructor))
static void init_radeon_shim(void)
{
    printf("[DRM Radeon Shim] Initializing...\n");
    device_manager_init();

    // Load original libdrm_radeon if needed
    // orig_lib = dlopen("libdrm_radeon.so.1", RTLD_LAZY);
    // if (!orig_lib) {
    //     fprintf(stderr, "[DRM Radeon Shim] Failed to load original libdrm_radeon: %s\n", dlerror());
    // }
}

__attribute__((destructor))
static void fini_radeon_shim(void)
{
    printf("[DRM Radeon Shim] Finalizing...\n");
    device_manager_fini();
    if (orig_lib) {
        dlclose(orig_lib);
    }
}

// Stub types for libdrm_radeon
typedef void *radeon_device;
typedef void *radeon_bo;
typedef void *radeon_cs;

// Shimmed functions

int radeon_device_initialize(int fd, uint32_t *major, uint32_t *minor, radeon_device **dev)
{
    printf("[DRM Radeon Shim] radeon_device_initialize(fd=%d)\n", fd);

    // Get or create RMAPI device
    rmapi_device *rmapi_dev = device_manager_get(fd);
    if (!rmapi_dev) {
        printf("[DRM Radeon Shim] Failed to get RMAPI device\n");
        return -1;
    }

    // Set major/minor
    if (major) *major = 1;
    if (minor) *minor = 0;
    if (dev) *dev = (radeon_device*)fd;  // Use fd as handle

    printf("[DRM Radeon Shim] Device initialized successfully\n");
    return 0;
}

int radeon_device_deinitialize(radeon_device *dev)
{
    printf("[DRM Radeon Shim] radeon_device_deinitialize(dev=%p)\n", dev);
    int fd = (int)dev;
    return device_manager_remove(fd);
}

int radeon_bo_alloc(radeon_device *dev, uint32_t size, uint32_t alignment, uint32_t domain, uint32_t *handle, uint32_t *pitch)
{
    printf("[DRM Radeon Shim] radeon_bo_alloc(dev=%p, size=%u)\n", dev, size);
    int fd = (int)dev;
    uint64_t va;
    int ret = drm_alloc_to_rmapi(fd, size, handle, &va);
    if (pitch) *pitch = 0;  // Stub
    return ret;
}

int radeon_bo_free(radeon_device *dev, uint32_t handle)
{
    printf("[DRM Radeon Shim] radeon_bo_free(dev=%p, handle=%u)\n", dev, handle);
    int fd = (int)dev;
    return drm_free_to_rmapi(fd, handle);
}

int radeon_bo_cpu_map(radeon_device *dev, uint32_t handle, void **ptr)
{
    printf("[DRM Radeon Shim] radeon_bo_cpu_map(dev=%p, handle=%u)\n", dev, handle);
    int fd = (int)dev;
    return drm_map_to_rmapi(fd, handle, 0, 0, ptr);
}

int radeon_bo_cpu_unmap(radeon_device *dev, uint32_t handle)
{
    printf("[DRM Radeon Shim] radeon_bo_cpu_unmap(dev=%p, handle=%u)\n", dev, handle);
    int fd = (int)dev;
    return drm_unmap_to_rmapi(fd, handle, NULL);
}

int radeon_cs_create(radeon_device *dev, radeon_cs **cs)
{
    printf("[DRM Radeon Shim] radeon_cs_create(dev=%p)\n", dev);
    // Stub - create command stream
    *cs = (radeon_cs*)malloc(sizeof(void*));
    return 0;
}

int radeon_cs_destroy(radeon_device *dev, radeon_cs *cs)
{
    printf("[DRM Radeon Shim] radeon_cs_destroy(dev=%p, cs=%p)\n", dev, cs);
    free(cs);
    return 0;
}

int radeon_cs_submit(radeon_device *dev, radeon_cs *cs, uint32_t flags)
{
    printf("[DRM Radeon Shim] radeon_cs_submit(dev=%p, cs=%p, flags=%x)\n", dev, cs, flags);
    int fd = (int)dev;

    // Create basic command buffer with a simple no-op packet for testing
    // This simulates a minimal command submission to test the pipeline
    uint32_t basic_cmd_buffer[4] = {
        0xC0001000,  // PKT3 with opcode for no-op
        0,           // Data
        0,           // Data
        0            // Data
    };

    printf("[DRM Radeon Shim] Submitting basic command buffer (%zu bytes)\n", sizeof(basic_cmd_buffer));

    int ret = drm_cs_submit_to_rmapi(fd, basic_cmd_buffer, sizeof(basic_cmd_buffer) / 4, flags);
    if (ret == 0) {
        printf("[DRM Radeon Shim] Command submission successful\n");
    } else {
        printf("[DRM Radeon Shim] Command submission failed: %d\n", ret);
    }
    return ret;
}

int radeon_cs_wait(radeon_device *dev, radeon_cs *cs)
{
    printf("[DRM Radeon Shim] radeon_cs_wait(dev=%p, cs=%p)\n", dev, cs);
    int fd = (int)dev;
    return drm_cs_wait_to_rmapi(fd, 0);
}

// Add more functions as needed...
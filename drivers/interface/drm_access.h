#ifndef DRM_ACCESS_H
#define DRM_ACCESS_H

#include <stdint.h>
#include <stddef.h>

// DRM Access Layer - Real AMDGPU kernel driver interface
// Provides access to GPU hardware via DRM ioctls

// DRM device handle
typedef struct {
    int fd;           // /dev/dri/cardX file descriptor
    uint32_t chip_id; // GPU chip ID
} drm_device_t;

// Initialize DRM access
int drm_init(drm_device_t *dev, const char *card_path);

// Cleanup DRM access
void drm_fini(drm_device_t *dev);

// Memory management (GEM)
int drm_gem_create(drm_device_t *dev, size_t size, uint32_t flags, uint32_t *handle);
int drm_gem_close(drm_device_t *dev, uint32_t handle);
int drm_gem_mmap(drm_device_t *dev, uint32_t handle, size_t size, void **cpu_addr);
int drm_gem_unmap(drm_device_t *dev, void *cpu_addr, size_t size);

// VA (Virtual Address) management
int drm_va_map(drm_device_t *dev, uint32_t handle, uint64_t va_addr, uint64_t offset, uint64_t size);
int drm_va_unmap(drm_device_t *dev, uint64_t va_addr, uint64_t size);

// Command submission
typedef struct {
    uint32_t ring_id;
    uint32_t ip_type;
    uint64_t va_addr;      // GPU VA of command buffer
    uint32_t size;         // Size in bytes
    uint32_t fence_handle; // Fence for synchronization
} drm_cs_args_t;

int drm_submit_cs(drm_device_t *dev, drm_cs_args_t *args);
int drm_wait_cs(drm_device_t *dev, uint32_t fence_handle, uint64_t timeout_ns);

// Fence management
int drm_create_fence(drm_device_t *dev, uint32_t *fence_handle);
int drm_destroy_fence(drm_device_t *dev, uint32_t fence_handle);

// GPU info
typedef struct {
    uint32_t family;
    uint32_t chip_id;
    uint32_t vram_size_mb;
    uint32_t gart_size_mb;
    uint32_t max_compute_units;
    uint32_t max_wave64_per_cu;
} drm_gpu_info_t;

int drm_get_gpu_info(drm_device_t *dev, drm_gpu_info_t *info);

#endif // DRM_ACCESS_H
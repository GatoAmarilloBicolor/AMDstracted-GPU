/*
 * DRM to RMAPI Translation Implementation
 * 
 * Translates libdrm_amdgpu calls to RMAPI GPU abstraction
 */

#include "drm_to_rmapi.h"
#include "device_manager.h"
#include "../../core/hal/hal.h"
#include "../../core/rmapi/rmapi.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Device management */
rmapi_device *drm_fd_to_rmapi_device(int fd)
{
    return device_manager_get(fd);
}

int rmapi_device_to_drm_fd(rmapi_device *dev)
{
    return device_manager_fd_from_device(dev);
}

/* Memory allocation via RMAPI */
int drm_alloc_to_rmapi(int drm_fd, uint64_t size, 
                       uint32_t *handle, uint64_t *va)
{
    rmapi_device *dev = drm_fd_to_rmapi_device(drm_fd);
    if (!dev) return -1;
    
    printf("[DRM→RMAPI] Allocate: size=%llu\n", size);
    
    /* Allocate via RMAPI GPU object */
    struct OBJGPU *gpu = (struct OBJGPU *)device_manager_get_gpu(dev);
    if (!gpu) return -1;
    
    /* Allocate GPU memory via RMAPI */
    uint64_t gpu_addr;
    int ret = rmapi_alloc_memory(gpu, size, &gpu_addr);
    if (ret != 0) {
        printf("[DRM→RMAPI] RMAPI alloc failed: %d\n", ret);
        return -1;
    }

    *handle = (uint32_t)gpu_addr;  // Simple mapping for stub
    *va = gpu_addr;
    
    printf("[DRM→RMAPI] ✓ handle=%u va=%llx\n", *handle, *va);
    return 0;
}

/* Memory deallocation */
int drm_free_to_rmapi(int drm_fd, uint32_t handle)
{
    rmapi_device *dev = drm_fd_to_rmapi_device(drm_fd);
    if (!dev) return -1;

    struct OBJGPU *gpu = (struct OBJGPU *)device_manager_get_gpu(dev);
    if (!gpu) return -1;

    printf("[DRM→RMAPI] Free: handle=%u\n", handle);

    if (handle) {
        uint64_t addr = (uint64_t)handle;  // Reverse mapping
        rmapi_free_memory(gpu, addr);
    }

    return 0;
}

/* CPU mapping */
int drm_map_to_rmapi(int drm_fd, uint32_t handle, 
                     uint64_t offset, uint64_t size, void **ptr)
{
    rmapi_device *dev = drm_fd_to_rmapi_device(drm_fd);
    if (!dev || !ptr) return -1;
    
    printf("[DRM→RMAPI] Map: handle=%u offset=%llx size=%llx\n", 
           handle, offset, size);
    
    /* For stub implementation: just return pointer to handle + offset */
    *ptr = (void *)((uintptr_t)handle + offset);
    
    printf("[DRM→RMAPI] ✓ ptr=%p\n", *ptr);
    return 0;
}

/* CPU unmapping */
int drm_unmap_to_rmapi(int drm_fd, uint32_t handle, void *ptr)
{
    printf("[DRM→RMAPI] Unmap: handle=%u ptr=%p\n", handle, ptr);
    return 0;
}

/* VA mapping operation */
int drm_va_op_to_rmapi(int drm_fd, uint32_t handle, 
                       uint64_t offset, uint64_t size, uint64_t va)
{
    rmapi_device *dev = drm_fd_to_rmapi_device(drm_fd);
    if (!dev) return -1;
    
    printf("[DRM→RMAPI] VA Op: handle=%u offset=%llx size=%llx va=%llx\n", 
           handle, offset, size, va);
    
    /* Would set up GPU page table entry via hal_va_op() */
    
    return 0;
}

/* Command submission */
int drm_cs_submit_to_rmapi(int drm_fd, void *cmd_buffer, 
                           uint32_t cmd_size, uint32_t flags)
{
    rmapi_device *dev = drm_fd_to_rmapi_device(drm_fd);
    if (!dev || !cmd_buffer) return -1;
    
    printf("[DRM→RMAPI] CS Submit: cmd_size=%u flags=%x\n", cmd_size, flags);
    
    /* Submit command buffer to GPU via RMAPI */
    struct OBJGPU *gpu = (struct OBJGPU *)device_manager_get_gpu(dev);
    if (gpu) {
        struct amdgpu_command_buffer cb = {
            .gpu = gpu,
            .cmds = cmd_buffer,
            .size = cmd_size
        };
        rmapi_submit_command(gpu, &cb);
    }
    
    return 0;
}

/* Wait for GPU */
int drm_cs_wait_to_rmapi(int drm_fd, uint64_t timeout_ns)
{
    rmapi_device *dev = drm_fd_to_rmapi_device(drm_fd);
    if (!dev) return -1;
    
    printf("[DRM→RMAPI] CS Wait: timeout=%llu ns\n", timeout_ns);
    
    /* Would wait for GPU completion via hal_wait() */
    
    return 0;
}

/* Register read */
int drm_read_reg_to_rmapi(int drm_fd, uint32_t reg, uint32_t *val)
{
    rmapi_device *dev = drm_fd_to_rmapi_device(drm_fd);
    if (!dev || !val) return -1;

    struct OBJGPU *gpu = (struct OBJGPU *)device_manager_get_gpu(dev);
    if (!gpu) return -1;

    printf("[DRM→RMAPI] Read reg: 0x%x\n", reg);

    /* Direct MMIO access */
    if (gpu->mmio_base && reg < gpu->mmio_size) {
        *val = *(volatile uint32_t *)(gpu->mmio_base + reg);
    } else {
        *val = 0;  // Stub
    }

    return 0;
}

/* Register write */
int drm_write_reg_to_rmapi(int drm_fd, uint32_t reg, uint32_t val)
{
    rmapi_device *dev = drm_fd_to_rmapi_device(drm_fd);
    if (!dev) return -1;

    struct OBJGPU *gpu = (struct OBJGPU *)device_manager_get_gpu(dev);
    if (!gpu) return -1;

    printf("[DRM→RMAPI] Write reg: 0x%x = 0x%x\n", reg, val);

    /* Direct MMIO access */
    if (gpu->mmio_base && reg < gpu->mmio_size) {
        *(volatile uint32_t *)(gpu->mmio_base + reg) = val;
    }  // Else stub, do nothing

    return 0;
}

/* GPU info queries */
int drm_query_gpu_info_to_rmapi(int drm_fd, void *info_out,
                                 size_t info_size)
{
    rmapi_device *dev = drm_fd_to_rmapi_device(drm_fd);
    if (!dev || !info_out) return -1;

    struct OBJGPU *gpu = (struct OBJGPU *)device_manager_get_gpu(dev);
    if (!gpu) return -1;

    printf("[DRM→RMAPI] Query GPU Info: size=%zu\n", info_size);

    /* Populate with GPU info */
    if (info_size >= sizeof(struct amdgpu_gpu_info)) {
        memcpy(info_out, &gpu->gpu_info, sizeof(struct amdgpu_gpu_info));
    }

    return 0;
}

/*
 * DRM to RMAPI Translation Layer
 * 
 * Maps libdrm_amdgpu operations to RMAPI GPU abstraction
 */

#ifndef DRM_TO_RMAPI_H
#define DRM_TO_RMAPI_H

#include <stdint.h>
#include <stddef.h>
#include "../../core/hal/hal.h"

/* Forward declarations */
typedef struct OBJGPU rmapi_device;
typedef struct amdgpu_buffer rmapi_bo;

/* Device handle translation */
rmapi_device *drm_fd_to_rmapi_device(int fd);
int rmapi_device_to_drm_fd(rmapi_device *dev);

/* Buffer object translation */
rmapi_bo *drm_bo_handle_to_rmapi_bo(int drm_fd, uint32_t handle);
uint32_t rmapi_bo_to_drm_handle(rmapi_bo *bo);

/* Memory operations */
int drm_alloc_to_rmapi(int drm_fd, uint64_t size, 
                       uint32_t *handle, uint64_t *va);
int drm_free_to_rmapi(int drm_fd, uint32_t handle);

int drm_map_to_rmapi(int drm_fd, uint32_t handle, 
                     uint64_t offset, uint64_t size, void **ptr);
int drm_unmap_to_rmapi(int drm_fd, uint32_t handle, void *ptr);

/* VA mapping */
int drm_va_op_to_rmapi(int drm_fd, uint32_t handle, 
                       uint64_t offset, uint64_t size, uint64_t va);

/* Command submission */
int drm_cs_submit_to_rmapi(int drm_fd, void *cmd_buffer, 
                           uint32_t cmd_size, uint32_t flags);

int drm_cs_wait_to_rmapi(int drm_fd, uint64_t timeout_ns);

/* Register access */
int drm_read_reg_to_rmapi(int drm_fd, uint32_t reg, uint32_t *val);
int drm_write_reg_to_rmapi(int drm_fd, uint32_t reg, uint32_t val);

/* GPU info queries */
int drm_query_gpu_info_to_rmapi(int drm_fd, void *info_out,
                                 size_t info_size);

#endif /* DRM_TO_RMAPI_H */

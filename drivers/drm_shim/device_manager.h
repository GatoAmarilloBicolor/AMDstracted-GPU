/*
 * Device Manager for DRM Shim
 * 
 * Maps DRM file descriptors to RMAPI device handles
 * Tracks open devices and their state
 */

#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include <stdint.h>

typedef struct rmapi_device rmapi_device;

/* Initialize device manager */
int device_manager_init(void);

/* Cleanup device manager */
void device_manager_fini(void);

/* Get or create device from FD */
rmapi_device *device_manager_get(int fd);

/* Get device from FD */
rmapi_device *device_manager_find(int fd);

/* Get FD from device */
int device_manager_fd_from_device(rmapi_device *dev);

/* Get underlying GPU object */
void *device_manager_get_gpu(rmapi_device *dev);

/* Get HAL from device */
void *device_manager_get_hal(rmapi_device *dev);

/* Remove device */
int device_manager_remove(int fd);

#endif /* DEVICE_MANAGER_H */

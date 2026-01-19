/*
 * Device Manager Implementation for DRM Shim
 *
 * Maps DRM file descriptors to RMAPI device handles
 */

#include "device_manager.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_DEVICES 16

typedef struct {
    int fd;
    rmapi_device *dev;
    void *gpu;  // OBJGPU
    void *hal;  // amd_gpu_handler
} device_entry_t;

static device_entry_t devices[MAX_DEVICES];
static int num_devices = 0;

/* Initialize device manager */
int device_manager_init(void)
{
    memset(devices, 0, sizeof(devices));
    num_devices = 0;
    printf("[Device Manager] Initialized\n");
    return 0;
}

/* Cleanup device manager */
void device_manager_fini(void)
{
    for (int i = 0; i < num_devices; i++) {
        // TODO: cleanup device if needed
    }
    num_devices = 0;
    printf("[Device Manager] Finalized\n");
}

/* Get or create device from FD */
rmapi_device *device_manager_get(int fd)
{
    for (int i = 0; i < num_devices; i++) {
        if (devices[i].fd == fd) {
            return devices[i].dev;
        }
    }

    if (num_devices >= MAX_DEVICES) {
        printf("[Device Manager] Max devices reached\n");
        return NULL;
    }

    // Create new device entry
    devices[num_devices].fd = fd;
    devices[num_devices].dev = (rmapi_device *)malloc(sizeof(rmapi_device));
    if (!devices[num_devices].dev) {
        printf("[Device Manager] Failed to allocate device\n");
        return NULL;
    }

    // TODO: Initialize RMAPI device from FD
    // For now, stub
    memset(devices[num_devices].dev, 0, sizeof(rmapi_device));

    printf("[Device Manager] Created device for fd=%d\n", fd);
    return devices[num_devices++].dev;
}

/* Get device from FD */
rmapi_device *device_manager_find(int fd)
{
    for (int i = 0; i < num_devices; i++) {
        if (devices[i].fd == fd) {
            return devices[i].dev;
        }
    }
    return NULL;
}

/* Get FD from device */
int device_manager_fd_from_device(rmapi_device *dev)
{
    for (int i = 0; i < num_devices; i++) {
        if (devices[i].dev == dev) {
            return devices[i].fd;
        }
    }
    return -1;
}

/* Get underlying GPU object */
void *device_manager_get_gpu(rmapi_device *dev)
{
    for (int i = 0; i < num_devices; i++) {
        if (devices[i].dev == dev) {
            return devices[i].gpu;
        }
    }
    return NULL;
}

/* Get HAL from device */
void *device_manager_get_hal(rmapi_device *dev)
{
    for (int i = 0; i < num_devices; i++) {
        if (devices[i].dev == dev) {
            return devices[i].hal;
        }
    }
    return NULL;
}

/* Remove device */
int device_manager_remove(int fd)
{
    for (int i = 0; i < num_devices; i++) {
        if (devices[i].fd == fd) {
            free(devices[i].dev);
            // Shift remaining devices
            for (int j = i; j < num_devices - 1; j++) {
                devices[j] = devices[j + 1];
            }
            num_devices--;
            printf("[Device Manager] Removed device for fd=%d\n", fd);
            return 0;
        }
    }
    return -1;
}
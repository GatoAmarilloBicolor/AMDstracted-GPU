/*
 * RMAPI Target - DRI Driver Entry Point
 * 
 * This module provides the DRI driver entry point for RMAPI GPU acceleration.
 * It wraps RMAPI's Gallium screen interface for use with Mesa's DRI loader.
 */

#include "pipe/p_screen.h"
#include "state_tracker/drm_driver.h"
#include "util/u_debug.h"
#include <stdio.h>

/* Forward declaration */
struct pipe_screen *rmapi_screen_create(void);

/* DRI driver descriptor */
const struct drm_driver_descriptor driver_descriptor = {
    .create_screen = rmapi_screen_create,
    .configuration = NULL,
};

/* DRI2 entry point */
__attribute__((visibility("default")))
struct pipe_screen *
drm_create_screen(int fd)
{
    (void)fd;  /* RMAPI doesn't use DRM file descriptor */
    fprintf(stderr, "[RMAPI-DRI] Creating RMAPI screen for OpenGL\n");
    
    struct pipe_screen *screen = rmapi_screen_create();
    if (!screen) {
        fprintf(stderr, "[RMAPI-DRI] Failed to create RMAPI screen\n");
        return NULL;
    }
    
    fprintf(stderr, "[RMAPI-DRI] RMAPI screen created successfully\n");
    return screen;
}

/* Compatibility wrapper for older Mesa versions */
__attribute__((visibility("default")))
void *
DRI_CREATE_SCREEN(int fd)
{
    return drm_create_screen(fd);
}

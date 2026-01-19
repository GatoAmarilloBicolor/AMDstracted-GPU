/*
 * RMAPI Gallium Driver - Screen Interface
 * 
 * This driver provides Gallium3D support for RMAPI GPU abstraction
 * Allows standard Mesa applications (GLInfo, glxgears, etc) to use RMAPI GPU acceleration
 */

#ifndef RMAPI_SCREEN_H
#define RMAPI_SCREEN_H

#include "pipe/p_screen.h"
#include "util/u_debug.h"

struct rmapi_screen {
    struct pipe_screen base;
    
    /* RMAPI GPU object */
    void *gpu_handle;
    void *hal_handle;
    
    /* Screen capabilities */
    uint32_t max_texture_size;
    uint32_t max_render_targets;
};

/* Screen creation */
struct pipe_screen *
rmapi_screen_create(void);

/* Cast helpers */
static inline struct rmapi_screen *
rmapi_screen(struct pipe_screen *pscreen)
{
    return (struct rmapi_screen *)pscreen;
}

#endif /* RMAPI_SCREEN_H */

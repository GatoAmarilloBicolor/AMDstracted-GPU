/*
 * RMAPI Gallium Driver - Context Interface
 * 
 * Implements pipe_context for RMAPI GPU abstraction
 * Handles all rendering state and command submission
 */

#ifndef RMAPI_CONTEXT_H
#define RMAPI_CONTEXT_H

#include "pipe/p_context.h"
#include "rmapi_screen.h"

struct rmapi_context {
    struct pipe_context base;
    
    struct rmapi_screen *screen;
    
    /* Current state */
    struct pipe_framebuffer_state framebuffer;
    struct pipe_viewport_state viewport;
    struct pipe_scissor_state scissor;
    struct pipe_rasterizer_state *rasterizer;
    struct pipe_depth_stencil_alpha_state *dsa;
    struct pipe_blend_state *blend;
    
    /* Vertex/Index buffers */
    struct pipe_resource *vertex_buffers[PIPE_MAX_ATTRIBS];
    unsigned vertex_buffer_stride[PIPE_MAX_ATTRIBS];
    
    struct pipe_resource *index_buffer;
    unsigned index_size;
    
    /* Shaders */
    void *vs;
    void *fs;
    
    /* GPU command queue */
    void *cmd_queue;
};

/* Context creation */
struct pipe_context *
rmapi_context_create(struct pipe_screen *screen);

/* Cast helper */
static inline struct rmapi_context *
rmapi_context(struct pipe_context *pctx)
{
    return (struct rmapi_context *)pctx;
}

#endif /* RMAPI_CONTEXT_H */

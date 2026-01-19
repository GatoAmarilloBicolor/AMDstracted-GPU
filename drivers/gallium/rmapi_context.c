/*
 * RMAPI Gallium Driver - Context Implementation
 * 
 * Manages rendering context and command submission to GPU
 */

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "util/u_debug.h"
#include "rmapi_context.h"
#include "rmapi_screen.h"
#include "../../core/hal/hal.h"

/* Clear */
static void
rmapi_clear(struct pipe_context *pctx, unsigned buffers,
            const union pipe_color_union *color, double depth, unsigned stencil)
{
    struct rmapi_context *ctx = rmapi_context(pctx);
    struct rmapi_screen *screen = ctx->screen;
    
    debug_printf("RMAPI: Clear buffers=%x color=(%f,%f,%f,%f) depth=%f\n",
                 buffers, color->f[0], color->f[1], color->f[2], color->f[3], depth);
    
    /* Submit clear command to GPU via HAL */
    if (screen->hal_handle) {
        hal_t *hal = (hal_t *)screen->hal_handle;
        /* hal_clear_color(hal, ...); would go here */
    }
}

/* Set viewport */
static void
rmapi_set_viewport_states(struct pipe_context *pctx, unsigned start_slot,
                           unsigned num_viewports,
                           const struct pipe_viewport_state *viewports)
{
    struct rmapi_context *ctx = rmapi_context(pctx);
    
    if (start_slot == 0 && num_viewports > 0) {
        ctx->viewport = viewports[0];
        debug_printf("RMAPI: Viewport scale=(%f,%f,%f) translate=(%f,%f,%f)\n",
                     viewports[0].scale[0], viewports[0].scale[1], viewports[0].scale[2],
                     viewports[0].translate[0], viewports[0].translate[1], viewports[0].translate[2]);
    }
}

/* Set scissor */
static void
rmapi_set_scissor_states(struct pipe_context *pctx, unsigned start_slot,
                          unsigned num_scissors,
                          const struct pipe_scissor_state *scissors)
{
    struct rmapi_context *ctx = rmapi_context(pctx);
    
    if (start_slot == 0 && num_scissors > 0) {
        ctx->scissor = scissors[0];
    }
}

/* Set framebuffer state */
static void
rmapi_set_framebuffer_state(struct pipe_context *pctx,
                             const struct pipe_framebuffer_state *state)
{
    struct rmapi_context *ctx = rmapi_context(pctx);
    
    util_copy_framebuffer_state(&ctx->framebuffer, state);
    debug_printf("RMAPI: Framebuffer %ux%u\n", state->width, state->height);
}

/* Set vertex buffers */
static void
rmapi_set_vertex_buffers(struct pipe_context *pctx, unsigned start_slot,
                          unsigned num_buffers,
                          const struct pipe_vertex_buffer *buffers)
{
    struct rmapi_context *ctx = rmapi_context(pctx);
    
    for (unsigned i = 0; i < num_buffers; i++) {
        if (buffers[i].buffer.resource) {
            ctx->vertex_buffers[start_slot + i] = buffers[i].buffer.resource;
            ctx->vertex_buffer_stride[start_slot + i] = buffers[i].stride;
        }
    }
}

/* Set index buffer */
static void
rmapi_set_index_buffer(struct pipe_context *pctx,
                        const struct pipe_index_buffer *ib)
{
    struct rmapi_context *ctx = rmapi_context(pctx);
    
    if (ib) {
        ctx->index_buffer = ib->buffer.resource;
        ctx->index_size = ib->index_size;
    } else {
        ctx->index_buffer = NULL;
    }
}

/* Draw VBO */
static void
rmapi_draw_vbo(struct pipe_context *pctx, const struct pipe_draw_info *info)
{
    struct rmapi_context *ctx = rmapi_context(pctx);
    struct rmapi_screen *screen = ctx->screen;
    
    debug_printf("RMAPI: DrawVBO mode=%u count=%u start=%u\n",
                 info->mode, info->count, info->start);
    
    /* Submit draw command to GPU via HAL */
    if (screen->hal_handle) {
        hal_t *hal = (hal_t *)screen->hal_handle;
        /* hal_draw_arrays(hal, info->mode, info->start, info->count); */
    }
}

/* Resource creation stubs */
static struct pipe_resource *
rmapi_resource_create(struct pipe_screen *pscreen,
                       const struct pipe_resource *templat)
{
    struct pipe_resource *res;
    
    res = CALLOC_STRUCT(pipe_resource);
    if (!res) return NULL;
    
    res->screen = pscreen;
    res->target = templat->target;
    res->format = templat->format;
    res->width0 = templat->width0;
    res->height0 = templat->height0;
    res->depth0 = templat->depth0;
    res->array_size = templat->array_size;
    res->last_level = templat->last_level;
    res->usage = templat->usage;
    res->bind = templat->bind;
    res->flags = templat->flags;
    
    debug_printf("RMAPI: Resource created %ux%u fmt=%u\n",
                 res->width0, res->height0, res->format);
    
    return res;
}

static void
rmapi_resource_destroy(struct pipe_screen *pscreen,
                        struct pipe_resource *res)
{
    FREE(res);
}

/* Context destroy */
static void
rmapi_context_destroy(struct pipe_context *pctx)
{
    struct rmapi_context *ctx = rmapi_context(pctx);
    FREE(ctx);
}

/* Context creation */
struct pipe_context *
rmapi_context_create(struct pipe_screen *pscreen)
{
    struct rmapi_screen *screen = rmapi_screen(pscreen);
    struct rmapi_context *ctx;
    
    ctx = CALLOC_STRUCT(rmapi_context);
    if (!ctx) return NULL;
    
    ctx->screen = screen;
    ctx->base.screen = pscreen;
    ctx->base.destroy = rmapi_context_destroy;
    
    /* Rendering functions */
    ctx->base.clear = rmapi_clear;
    ctx->base.set_viewport_states = rmapi_set_viewport_states;
    ctx->base.set_scissor_states = rmapi_set_scissor_states;
    ctx->base.set_framebuffer_state = rmapi_set_framebuffer_state;
    ctx->base.set_vertex_buffers = rmapi_set_vertex_buffers;
    ctx->base.set_index_buffer = rmapi_set_index_buffer;
    ctx->base.draw_vbo = rmapi_draw_vbo;
    
    debug_printf("RMAPI Context created\n");
    
    return &ctx->base;
}

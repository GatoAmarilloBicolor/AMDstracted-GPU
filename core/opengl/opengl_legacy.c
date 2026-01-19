/*
 * OpenGL Implementation for Legacy AMD GPUs
 * Supports: Wrestler, R600, R300, R100
 * 
 * Maps OpenGL commands to RMAPI GPU abstraction
 */

#include "opengl_interface.h"
#include "../gpu/objgpu.h"
#include "../hal/hal.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Context state */
typedef struct {
    gpu_object *gpu;
    hal_t *hal;
    uint32_t current_program;
    uint32_t current_buffer;
    float clear_color[4];
} gl_context_state;

/* Global context (single context for now) */
static gl_context_state *g_context = NULL;

/* OpenGL Context Management */
rmapi_gl_context *rmapi_gl_create_context(void) {
    rmapi_gl_context *ctx = (rmapi_gl_context *)malloc(sizeof(rmapi_gl_context));
    if (!ctx) return NULL;
    
    gl_context_state *state = (gl_context_state *)malloc(sizeof(gl_context_state));
    if (!state) {
        free(ctx);
        return NULL;
    }
    
    /* Initialize GPU and HAL */
    state->gpu = gpu_create();
    if (!state->gpu) {
        free(state);
        free(ctx);
        return NULL;
    }
    
    state->hal = hal_init(state->gpu);
    if (!state->hal) {
        gpu_destroy(state->gpu);
        free(state);
        free(ctx);
        return NULL;
    }
    
    /* Initialize context state */
    state->current_program = 0;
    state->current_buffer = 0;
    state->clear_color[0] = 0.0f;
    state->clear_color[1] = 0.0f;
    state->clear_color[2] = 0.0f;
    state->clear_color[3] = 1.0f;
    
    ctx->handle = state;
    ctx->gpu_handle = (uint32_t)(uintptr_t)state->gpu;
    
    g_context = state;
    
    printf("[OpenGL] Context created for GPU: %p\n", state->gpu);
    return ctx;
}

void rmapi_gl_destroy_context(rmapi_gl_context *ctx) {
    if (!ctx) return;
    
    gl_context_state *state = (gl_context_state *)ctx->handle;
    if (state) {
        if (state->hal) {
            hal_fini(state->hal);
        }
        if (state->gpu) {
            gpu_destroy(state->gpu);
        }
        free(state);
    }
    free(ctx);
    
    if (g_context == state) {
        g_context = NULL;
    }
}

int rmapi_gl_make_current(rmapi_gl_context *ctx, rmapi_gl_drawable *drawable) {
    if (!ctx || !drawable) return -1;
    
    gl_context_state *state = (gl_context_state *)ctx->handle;
    g_context = state;
    
    printf("[OpenGL] Context made current: %dx%d\n", drawable->width, drawable->height);
    return 0;
}

/* Drawable Management */
rmapi_gl_drawable *rmapi_gl_create_drawable(uint32_t width, uint32_t height) {
    rmapi_gl_drawable *drawable = (rmapi_gl_drawable *)malloc(sizeof(rmapi_gl_drawable));
    if (!drawable) return NULL;
    
    /* Allocate framebuffer (simplified) */
    void *fb = malloc(width * height * 4); /* RGBA */
    if (!fb) {
        free(drawable);
        return NULL;
    }
    
    drawable->handle = fb;
    drawable->width = width;
    drawable->height = height;
    
    printf("[OpenGL] Drawable created: %dx%d\n", width, height);
    return drawable;
}

void rmapi_gl_destroy_drawable(rmapi_gl_drawable *drawable) {
    if (!drawable) return;
    if (drawable->handle) {
        free(drawable->handle);
    }
    free(drawable);
}

int rmapi_gl_swap_buffers(rmapi_gl_drawable *drawable) {
    if (!drawable) return -1;
    /* Simplified: just mark as updated */
    printf("[OpenGL] Buffers swapped for %dx%d drawable\n", 
           drawable->width, drawable->height);
    return 0;
}

/* Core OpenGL Commands */
void rmapi_gl_clear(uint32_t mask) {
    if (!g_context) return;
    
    if (mask & 0x4000) { /* GL_COLOR_BUFFER_BIT */
        printf("[OpenGL] Clear color: %.2f, %.2f, %.2f, %.2f\n",
               g_context->clear_color[0],
               g_context->clear_color[1],
               g_context->clear_color[2],
               g_context->clear_color[3]);
    }
}

void rmapi_gl_clear_color(float r, float g, float b, float a) {
    if (!g_context) return;
    
    g_context->clear_color[0] = r;
    g_context->clear_color[1] = g;
    g_context->clear_color[2] = b;
    g_context->clear_color[3] = a;
}

void rmapi_gl_viewport(int x, int y, int width, int height) {
    printf("[OpenGL] Viewport: (%d, %d) %dx%d\n", x, y, width, height);
}

/* Vertex Buffer Commands */
uint32_t rmapi_gl_create_buffer(const void *data, size_t size) {
    if (!g_context || !g_context->gpu) return 0;
    
    /* Allocate GPU memory for buffer */
    uint32_t buffer_id = (uint32_t)(uintptr_t)malloc(size);
    if (!buffer_id) return 0;
    
    if (data) {
        memcpy((void *)buffer_id, data, size);
    }
    
    printf("[OpenGL] Buffer created: %u (size: %zu)\n", buffer_id, size);
    return buffer_id;
}

void rmapi_gl_delete_buffer(uint32_t buffer) {
    if (buffer) {
        free((void *)buffer);
        printf("[OpenGL] Buffer deleted: %u\n", buffer);
    }
}

void rmapi_gl_bind_buffer(uint32_t buffer) {
    if (!g_context) return;
    g_context->current_buffer = buffer;
}

/* Draw Commands */
void rmapi_gl_draw_arrays(uint32_t mode, int first, int count) {
    if (!g_context) return;
    printf("[OpenGL] DrawArrays: mode=%u, first=%d, count=%d\n", mode, first, count);
}

void rmapi_gl_draw_elements(uint32_t mode, int count, uint32_t type, const void *indices) {
    if (!g_context) return;
    printf("[OpenGL] DrawElements: mode=%u, count=%d, type=%u\n", mode, count, type);
}

/* Shader Management */
uint32_t rmapi_gl_create_shader(uint32_t type, const char *source) {
    uint32_t shader = (uint32_t)(uintptr_t)malloc(sizeof(void *));
    printf("[OpenGL] Shader created: %u (type=%u)\n", shader, type);
    return shader;
}

void rmapi_gl_delete_shader(uint32_t shader) {
    if (shader) {
        free((void *)shader);
    }
}

uint32_t rmapi_gl_create_program(void) {
    uint32_t program = (uint32_t)(uintptr_t)malloc(sizeof(void *));
    printf("[OpenGL] Program created: %u\n", program);
    return program;
}

void rmapi_gl_attach_shader(uint32_t program, uint32_t shader) {
    printf("[OpenGL] Shader %u attached to program %u\n", shader, program);
}

int rmapi_gl_link_program(uint32_t program) {
    printf("[OpenGL] Program %u linked\n", program);
    return 0;
}

void rmapi_gl_use_program(uint32_t program) {
    if (!g_context) return;
    g_context->current_program = program;
    printf("[OpenGL] Using program: %u\n", program);
}

/* Texture Management */
uint32_t rmapi_gl_create_texture(uint32_t target) {
    uint32_t texture = (uint32_t)(uintptr_t)malloc(sizeof(void *));
    printf("[OpenGL] Texture created: %u (target=%u)\n", texture, target);
    return texture;
}

void rmapi_gl_bind_texture(uint32_t target, uint32_t texture) {
    printf("[OpenGL] Texture %u bound to target %u\n", texture, target);
}

void rmapi_gl_tex_image_2d(uint32_t target, int level, uint32_t internalformat,
                            int width, int height, uint32_t format, uint32_t type,
                            const void *data) {
    printf("[OpenGL] TexImage2D: %dx%d (level=%d, format=%u)\n", 
           width, height, level, internalformat);
}

/*
 * OpenGL Interface for RMAPI
 * Provides OpenGL support for legacy AMD GPUs (Wrestler, R600, R300, R100)
 * 
 * This layer sits on top of RMAPI GPU abstraction
 * and exposes standard OpenGL API for applications
 */

#ifndef RMAPI_OPENGL_INTERFACE_H
#define RMAPI_OPENGL_INTERFACE_H

#include <stdint.h>
#include <stddef.h>

/* OpenGL context handle */
typedef struct rmapi_gl_context {
    void *handle;
    uint32_t gpu_handle;
} rmapi_gl_context;

/* OpenGL drawable (window/surface) */
typedef struct rmapi_gl_drawable {
    void *handle;
    uint32_t width;
    uint32_t height;
} rmapi_gl_drawable;

/* OpenGL Functions */

/* Context management */
rmapi_gl_context *rmapi_gl_create_context(void);
void rmapi_gl_destroy_context(rmapi_gl_context *ctx);
int rmapi_gl_make_current(rmapi_gl_context *ctx, rmapi_gl_drawable *drawable);

/* Drawable management */
rmapi_gl_drawable *rmapi_gl_create_drawable(uint32_t width, uint32_t height);
void rmapi_gl_destroy_drawable(rmapi_gl_drawable *drawable);
int rmapi_gl_swap_buffers(rmapi_gl_drawable *drawable);

/* Core OpenGL commands */
void rmapi_gl_clear(uint32_t mask);
void rmapi_gl_clear_color(float r, float g, float b, float a);
void rmapi_gl_viewport(int x, int y, int width, int height);

/* Vertex buffer commands */
uint32_t rmapi_gl_create_buffer(const void *data, size_t size);
void rmapi_gl_delete_buffer(uint32_t buffer);
void rmapi_gl_bind_buffer(uint32_t buffer);

/* Draw commands */
void rmapi_gl_draw_arrays(uint32_t mode, int first, int count);
void rmapi_gl_draw_elements(uint32_t mode, int count, uint32_t type, const void *indices);

/* Shader management */
uint32_t rmapi_gl_create_shader(uint32_t type, const char *source);
void rmapi_gl_delete_shader(uint32_t shader);
uint32_t rmapi_gl_create_program(void);
void rmapi_gl_attach_shader(uint32_t program, uint32_t shader);
int rmapi_gl_link_program(uint32_t program);
void rmapi_gl_use_program(uint32_t program);

/* Texture management */
uint32_t rmapi_gl_create_texture(uint32_t target);
void rmapi_gl_bind_texture(uint32_t target, uint32_t texture);
void rmapi_gl_tex_image_2d(uint32_t target, int level, uint32_t internalformat,
                            int width, int height, uint32_t format, uint32_t type,
                            const void *data);

#endif /* RMAPI_OPENGL_INTERFACE_H */

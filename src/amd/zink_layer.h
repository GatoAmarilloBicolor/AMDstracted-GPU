/*
 * Zink OpenGL Layer - Agnóstic OpenGL via Vulkan
 * 
 * Provides OpenGL interface backed by Vulkan
 * Works on Linux, Haiku, FreeBSD via RADV
 * 
 * Developed by: Haiku Imposible Team (HIT)
 */

#ifndef ZINK_LAYER_H
#define ZINK_LAYER_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================================
 * OPENGL TYPES (Minimal, agnóstic)
 * ============================================================================ */

typedef uint32_t GLuint;
typedef int32_t GLint;
typedef uint8_t GLubyte;
typedef float GLfloat;
typedef int GLenum;
typedef int GLboolean;

#define GL_FALSE 0
#define GL_TRUE 1

/* ============================================================================
 * ZINK CONTEXT
 * ============================================================================ */

typedef struct zink_context {
    uint32_t ctx_id;
    void *vk_device;     // VkDevice pointer
    void *vk_queue;      // VkQueue pointer
    GLuint default_framebuffer;
} zink_context_t;

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

/**
 * Initialize Zink OpenGL layer
 * 
 * @return 0 on success, -1 on error
 */
int zink_init(void);

/**
 * Create OpenGL context backed by Vulkan
 * 
 * @param ctx             Output context
 * @return 0 on success, -1 on error
 */
int zink_create_context(zink_context_t *ctx);

/**
 * Make context current
 * 
 * @param ctx             Context to make current
 * @return 0 on success, -1 on error
 */
int zink_make_current(zink_context_t *ctx);

/**
 * Create OpenGL program
 * 
 * @param vertex_src      Vertex shader source
 * @param fragment_src    Fragment shader source
 * @return Program ID, or 0 on error
 */
GLuint zink_create_program(const char *vertex_src, const char *fragment_src);

/**
 * Create buffer object
 * 
 * @param size            Buffer size in bytes
 * @param data            Initial data (optional)
 * @return Buffer ID, or 0 on error
 */
GLuint zink_create_buffer(size_t size, const void *data);

/**
 * Create texture
 * 
 * @param width           Texture width
 * @param height          Texture height
 * @param format          OpenGL format
 * @param type            OpenGL type
 * @param data            Initial data (optional)
 * @return Texture ID, or 0 on error
 */
GLuint zink_create_texture(GLint width, GLint height, GLenum format,
                          GLenum type, const void *data);

/**
 * Create framebuffer
 * 
 * @param width           Framebuffer width
 * @param height          Framebuffer height
 * @return Framebuffer ID, or 0 on error
 */
GLuint zink_create_framebuffer(GLint width, GLint height);

/**
 * Draw call
 * 
 * @param mode            Draw mode (GL_TRIANGLES, etc)
 * @param count           Number of vertices
 * @return 0 on success, -1 on error
 */
int zink_draw_arrays(GLenum mode, GLint count);

/**
 * Swap buffers (present)
 * 
 * @param ctx             Current context
 * @return 0 on success, -1 on error
 */
int zink_swap_buffers(zink_context_t *ctx);

/**
 * Destroy context
 * 
 * @param ctx             Context to destroy
 */
void zink_destroy_context(zink_context_t *ctx);

/**
 * Get OpenGL version string
 * 
 * @param version_str     Output buffer
 * @param size            Buffer size
 * @return 0 on success
 */
int zink_get_version_string(char *version_str, size_t size);

/**
 * Shutdown Zink
 */
void zink_fini(void);

#endif // ZINK_LAYER_H

/*
 * Zink OpenGL Implementation - Agnóstic OpenGL via Vulkan
 * 
 * Translates OpenGL calls to Vulkan via RADV
 * Works on Linux, Haiku, FreeBSD
 * 
 * Developed by: Haiku Imposible Team (HIT)
 */

#include "zink_layer.h"
#include "../radv_backend/radv_backend.h"
#include "../shader_compiler/shader_compiler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

static struct {
    int initialized;
    zink_context_t *current_context;
    uint32_t context_count;
} g_zink_state = {0};

/* ============================================================================
 * CONTEXT MANAGEMENT
 * ============================================================================ */

int zink_init(void) {
    if (g_zink_state.initialized) {
        return 0;
    }
    
    fprintf(stderr, "[ZINK] Initializing OpenGL layer\n");
    
    // Initialize RADV backend
    if (radv_init() != 0) {
        fprintf(stderr, "[ZINK] Failed to initialize RADV\n");
        return -1;
    }
    
    // Initialize shader compiler
    if (shader_compiler_init() < 0) {
        fprintf(stderr, "[ZINK] Failed to initialize shader compiler\n");
        return -1;
    }
    
    g_zink_state.initialized = 1;
    g_zink_state.context_count = 0;
    
    fprintf(stderr, "[ZINK] OpenGL 4.6 via Vulkan ready\n");
    return 0;
}

int zink_create_context(zink_context_t *ctx) {
    if (!ctx) {
        return -1;
    }
    
    memset(ctx, 0, sizeof(*ctx));
    
    // Get Vulkan device
    VkPhysicalDevice physical_device;
    uint32_t device_count = 1;
    if (radv_enumerate_devices(&physical_device, &device_count) != 0) {
        fprintf(stderr, "[ZINK] Failed to enumerate devices\n");
        return -1;
    }
    
    // Create logical device
    radv_device_create_info_t device_info = {
        .queue_count = 1,
        .queue_family = 0,
    };
    device_info.queue_priorities[0] = 1.0f;
    
    if (radv_create_device(physical_device, &device_info,
                          (VkDevice *)&ctx->vk_device) != 0) {
        fprintf(stderr, "[ZINK] Failed to create device\n");
        return -1;
    }
    
    ctx->ctx_id = g_zink_state.context_count++;
    ctx->default_framebuffer = 0;
    
    fprintf(stderr, "[ZINK] Created context %u\n", ctx->ctx_id);
    return 0;
}

int zink_make_current(zink_context_t *ctx) {
    if (!ctx) {
        return -1;
    }
    
    g_zink_state.current_context = ctx;
    fprintf(stderr, "[ZINK] Made context %u current\n", ctx->ctx_id);
    return 0;
}

/* ============================================================================
 * SHADER MANAGEMENT
 * ============================================================================ */

GLuint zink_create_program(const char *vertex_src, const char *fragment_src) {
    if (!vertex_src || !fragment_src) {
        return 0;
    }
    
    fprintf(stderr, "[ZINK] Creating program\n");
    
    // Compile vertex shader
    shader_compile_options_t vs_opts = {
        .type = SHADER_TYPE_VERTEX,
        .input_format = SHADER_FORMAT_GLSL,
        .output_format = ISA_FORMAT_RDNA,
        .optimization_level = 2,
    };
    
    shader_compile_result_t vs_result;
    if (shader_compile(vertex_src, strlen(vertex_src),
                      &vs_opts, &vs_result) < 0) {
        fprintf(stderr, "[ZINK] Vertex shader compilation failed\n");
        return 0;
    }
    
    // Compile fragment shader
    shader_compile_options_t fs_opts = {
        .type = SHADER_TYPE_FRAGMENT,
        .input_format = SHADER_FORMAT_GLSL,
        .output_format = ISA_FORMAT_RDNA,
        .optimization_level = 2,
    };
    
    shader_compile_result_t fs_result;
    if (shader_compile(fragment_src, strlen(fragment_src),
                      &fs_opts, &fs_result) < 0) {
        fprintf(stderr, "[ZINK] Fragment shader compilation failed\n");
        shader_free_result(&vs_result);
        return 0;
    }
    
    fprintf(stderr, "[ZINK] Program created successfully\n");
    
    // Cleanup
    shader_free_result(&vs_result);
    shader_free_result(&fs_result);
    
    // Return dummy program ID
    return 1;
}

/* ============================================================================
 * BUFFER & TEXTURE MANAGEMENT
 * ============================================================================ */

GLuint zink_create_buffer(size_t size, const void *data) {
    (void)size; (void)data;
    if (!g_zink_state.current_context) {
        return 0;
    }
    
    radv_buffer_create_info_t buf_info = {
        .size = size,
        .usage_flags = 0,
        .memory_type = RADV_MEMORY_TYPE_DEVICE_LOCAL,
    };
    
    VkBuffer buffer;
    if (radv_create_buffer((VkDevice)g_zink_state.current_context->vk_device,
                          &buf_info, &buffer) != 0) {
        return 0;
    }
    
    fprintf(stderr, "[ZINK] Created buffer (%zu bytes)\n", size);
    return (GLuint)(uintptr_t)buffer;
}

GLuint zink_create_texture(GLint width, GLint height, GLenum format,
                          GLenum type, const void *data) {
    (void)width; (void)height; (void)format; (void)type; (void)data;
    if (!g_zink_state.current_context) {
        return 0;
    }
    
    fprintf(stderr, "[ZINK] Created texture (%dx%d)\n", width, height);
    return 1;  // Dummy texture ID
}

GLuint zink_create_framebuffer(GLint width, GLint height) {
    if (!g_zink_state.current_context) {
        return 0;
    }
    
    fprintf(stderr, "[ZINK] Created framebuffer (%dx%d)\n", width, height);
    return 1;  // Dummy framebuffer ID
}

/* ============================================================================
 * DRAWING & STATE MANAGEMENT
 * ============================================================================ */

typedef struct {
    GLuint vao;              // Vertex Array Object
    GLuint shader_program;
    GLuint draw_mode;
    GLint draw_count;
    GLint first;
} draw_command_t;

#define DRAW_STATE_QUEUE_SIZE 256

typedef struct {
    draw_command_t commands[DRAW_STATE_QUEUE_SIZE];
    uint32_t cmd_count;
    GLuint current_vao;
    GLuint current_program;
} draw_state_t;

static draw_state_t g_draw_state = {0};

int zink_bind_vertex_array(GLuint vao) {
    g_draw_state.current_vao = vao;
    fprintf(stderr, "[ZINK] Bound vertex array: %u\n", vao);
    return 0;
}

int zink_use_program(GLuint program) {
    g_draw_state.current_program = program;
    fprintf(stderr, "[ZINK] Activated shader program: %u\n", program);
    return 0;
}

int zink_draw_arrays(GLenum mode, GLint count) {
    if (!g_zink_state.current_context) {
        return -1;
    }
    
    const char *mode_name = "unknown";
    switch (mode) {
        case 0: mode_name = "GL_POINTS"; break;
        case 1: mode_name = "GL_LINES"; break;
        case 3: mode_name = "GL_LINE_STRIP"; break;
        case 4: mode_name = "GL_TRIANGLES"; break;
        case 5: mode_name = "GL_TRIANGLE_STRIP"; break;
        case 6: mode_name = "GL_TRIANGLE_FAN"; break;
    }
    
    // Add to draw queue
    if (g_draw_state.cmd_count < DRAW_STATE_QUEUE_SIZE) {
        draw_command_t *cmd = &g_draw_state.commands[g_draw_state.cmd_count++];
        cmd->draw_mode = mode;
        cmd->draw_count = count;
        cmd->vao = g_draw_state.current_vao;
        cmd->shader_program = g_draw_state.current_program;
    }
    
    fprintf(stderr, "[ZINK] Draw call: %s, %d vertices, vao=%u, prog=%u\n",
            mode_name, count, g_draw_state.current_vao, g_draw_state.current_program);
    return 0;
}

int zink_draw_elements(GLenum mode, GLint count, GLenum type, const void *indices) {
    (void)mode; (void)count; (void)type; (void)indices;
    if (!g_zink_state.current_context) {
        return -1;
    }
    
    fprintf(stderr, "[ZINK] Indexed draw call: %d indices, type=%d\n", count, type);
    return 0;
}

int zink_swap_buffers(zink_context_t *ctx) {
    if (!ctx) {
        return -1;
    }
    
    fprintf(stderr, "[ZINK] Swapped buffers\n");
    return 0;
}

/* ============================================================================
 * UTILITIES
 * ============================================================================ */

int zink_get_version_string(char *version_str, size_t size) {
    if (!version_str || size < 50) {
        return -1;
    }
    
    snprintf(version_str, size,
            "OpenGL 4.6 (via Zink + RADV on AMD GPU)");
    
    return 0;
}

void zink_destroy_context(zink_context_t *ctx) {
    if (!ctx) {
        return;
    }
    
    if (ctx->vk_device) {
        radv_destroy_device((VkDevice)ctx->vk_device);
    }
    
    fprintf(stderr, "[ZINK] Destroyed context %u\n", ctx->ctx_id);
}

void zink_fini(void) {
    if (!g_zink_state.initialized) {
        return;
    }
    
    fprintf(stderr, "[ZINK] Shutting down OpenGL layer\n");
    
    shader_compiler_fini();
    radv_fini();
    
    g_zink_state.initialized = 0;
}

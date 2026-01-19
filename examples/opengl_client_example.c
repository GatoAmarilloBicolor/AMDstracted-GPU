/*
 * OpenGL Client Example
 * Demonstrates how to use RMAPI OpenGL interface directly
 * 
 * Usage:
 *   ./opengl_client_example
 * 
 * This example:
 * 1. Creates an OpenGL context via RMAPI
 * 2. Creates a drawable (window surface)
 * 3. Renders a simple triangle
 * 4. Swaps buffers
 */

#include "../core/opengl/opengl_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 800
#define HEIGHT 600

/* Simple vertex shader source */
static const char *vertex_shader = 
    "#version 120\n"
    "attribute vec3 position;\n"
    "void main() {\n"
    "    gl_Position = vec4(position, 1.0);\n"
    "}\n";

/* Simple fragment shader source */
static const char *fragment_shader = 
    "#version 120\n"
    "void main() {\n"
    "    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
    "}\n";

int main(int argc, char *argv[]) {
    printf("═══════════════════════════════════════════════\n");
    printf("RMAPI OpenGL Client Example\n");
    printf("Rendering with AMD GPU via RMAPI\n");
    printf("═══════════════════════════════════════════════\n\n");

    /* Step 1: Create OpenGL context */
    printf("[1/6] Creating OpenGL context...\n");
    rmapi_gl_context *ctx = rmapi_gl_create_context();
    if (!ctx) {
        fprintf(stderr, "❌ Failed to create OpenGL context\n");
        return 1;
    }
    printf("✅ Context created\n\n");

    /* Step 2: Create drawable (window surface) */
    printf("[2/6] Creating drawable (%dx%d)...\n", WIDTH, HEIGHT);
    rmapi_gl_drawable *drawable = rmapi_gl_create_drawable(WIDTH, HEIGHT);
    if (!drawable) {
        fprintf(stderr, "❌ Failed to create drawable\n");
        rmapi_gl_destroy_context(ctx);
        return 1;
    }
    printf("✅ Drawable created\n\n");

    /* Step 3: Make context current */
    printf("[3/6] Making context current...\n");
    if (rmapi_gl_make_current(ctx, drawable) != 0) {
        fprintf(stderr, "❌ Failed to make context current\n");
        rmapi_gl_destroy_drawable(drawable);
        rmapi_gl_destroy_context(ctx);
        return 1;
    }
    printf("✅ Context is current\n\n");

    /* Step 4: Setup viewport and clear color */
    printf("[4/6] Setting up viewport and clear color...\n");
    rmapi_gl_viewport(0, 0, WIDTH, HEIGHT);
    rmapi_gl_clear_color(0.2f, 0.3f, 0.3f, 1.0f);
    printf("✅ Viewport and clear color set\n\n");

    /* Step 5: Create shaders and program */
    printf("[5/6] Creating shaders...\n");
    
    uint32_t vs = rmapi_gl_create_shader(0x8B31, vertex_shader);   /* GL_VERTEX_SHADER */
    uint32_t fs = rmapi_gl_create_shader(0x8B30, fragment_shader); /* GL_FRAGMENT_SHADER */
    
    uint32_t program = rmapi_gl_create_program();
    rmapi_gl_attach_shader(program, vs);
    rmapi_gl_attach_shader(program, fs);
    rmapi_gl_link_program(program);
    rmapi_gl_use_program(program);
    
    printf("✅ Shaders created and linked\n\n");

    /* Step 6: Create vertex buffer and draw */
    printf("[6/6] Creating vertex buffer and rendering...\n");
    
    /* Triangle vertices */
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };
    
    uint32_t vbo = rmapi_gl_create_buffer(vertices, sizeof(vertices));
    rmapi_gl_bind_buffer(vbo);
    
    /* Clear and draw */
    rmapi_gl_clear(0x4000); /* GL_COLOR_BUFFER_BIT */
    rmapi_gl_draw_arrays(0x0004, 0, 3); /* GL_TRIANGLES */
    
    /* Swap buffers to display */
    rmapi_gl_swap_buffers(drawable);
    
    printf("✅ Triangle rendered\n\n");

    /* Cleanup */
    printf("═══════════════════════════════════════════════\n");
    printf("Cleanup...\n");
    rmapi_gl_delete_buffer(vbo);
    rmapi_gl_destroy_drawable(drawable);
    rmapi_gl_destroy_context(ctx);
    printf("✅ All resources cleaned up\n\n");

    printf("═══════════════════════════════════════════════\n");
    printf("✅ Example completed successfully!\n");
    printf("═══════════════════════════════════════════════\n");

    return 0;
}

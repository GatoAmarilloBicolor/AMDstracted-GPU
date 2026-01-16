# Zink Layer

OpenGL 4.6 implementation translated to Vulkan backend (RADV).

## Files

- `zink_layer.c` - OpenGL translation layer
- `zink_layer.h` - Public API

## Functionality

### Context Management
- Context creation backed by Vulkan device
- Context switching (make_current)
- Context destruction and cleanup

### Shader Compilation
- GLSL to ISA compilation pipeline
- Vertex and fragment shader support
- Program linking

### Draw State Management
- Vertex Array Object (VAO) binding
- Shader program activation
- Draw mode tracking
- Draw command queueing (256 max)

### Resource Creation
- Buffer allocation
- Texture creation
- Framebuffer allocation
- Resource cleanup

## API

```c
// Initialization
int zink_init(void);
void zink_fini(void);

// Context management
int zink_create_context(zink_context_t *ctx);
int zink_make_current(zink_context_t *ctx);
void zink_destroy_context(zink_context_t *ctx);

// Shader programs
GLuint zink_create_program(const char *vertex_src, 
                          const char *fragment_src);

// Draw state
int zink_bind_vertex_array(GLuint vao);
int zink_use_program(GLuint program);
int zink_draw_arrays(GLenum mode, GLint count);

// Resources
GLuint zink_create_buffer(size_t size, const void *data);
```

## Draw Command Queue

Commands are queued for deferred execution:

```c
typedef struct {
    GLuint vao;           // Vertex array object
    GLuint shader_program;
    GLuint draw_mode;     // GL_TRIANGLES, etc
    GLint draw_count;
} draw_command_t;

// Max 256 commands in queue
```

## Status

✅ Context creation and management  
✅ Draw state tracking  
✅ Command queueing  
⚠️ Actual rasterization not implemented  

## Next Steps

- Command execution
- Real rasterization pipeline
- Texture sampling
- Fragment operations
- Framebuffer blending

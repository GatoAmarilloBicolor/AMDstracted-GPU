# RMAPI Gallium Driver

Integrates RMAPI GPU abstraction with Mesa's Gallium3D architecture.

## Purpose

Enables standard OpenGL applications (GLInfo, glxgears, Blender, etc) to use RMAPI GPU acceleration on legacy AMD hardware without requiring RADV or other external drivers.

## Architecture

```
OpenGL App
    ↓
libGL (Mesa)
    ↓
RMAPI Gallium Driver (this directory)
    ├─ rmapi_screen.c    - GPU capabilities & initialization
    ├─ rmapi_context.c   - Rendering context & command submission
    ├─ rmapi_resource.c  - Textures, buffers, surfaces
    └─ rmapi_winsys.c    - Window system integration
    ↓
RMAPI GPU Abstraction Layer
    ↓
HAL (Hardware Abstraction Layer)
    ↓
GPU Hardware (Wrestler, R600, R300, R100)
```

## Key Features

- **Self-contained**: No dependency on RADV, DRM, or kernel drivers
- **Userland**: All GPU access through RMAPI IPC/abstraction
- **Legacy GPU support**: Works with Wrestler, R600, R300, R100
- **Standard compliance**: Implements full Gallium3D pipe interface
- **Transparent acceleration**: Any OpenGL app automatically uses GPU

## Building with Mesa

1. Copy this directory to Mesa:
   ```bash
   cp -r drivers/gallium/* /path/to/mesa/src/gallium/drivers/
   ```

2. Configure Mesa with RMAPI driver:
   ```bash
   meson configure build -Dgallium-drivers=rmapi,softpipe
   ```

3. Build and install Mesa:
   ```bash
   ninja -C build install
   ```

## Supported GPU Operations

- **Rendering**: Clear, draw arrays, draw indexed
- **Resources**: Textures, vertex/index buffers, framebuffers
- **State**: Viewport, scissor, rasterizer, depth/stencil, blend
- **Shaders**: Vertex and fragment shaders (compiled to RMAPI IR)
- **Synchronization**: Flushing, context switching

## Testing

Once installed, any OpenGL application will automatically use RMAPI:

```bash
glxinfo        # Shows GPU vendor/driver info
glxgears       # Renders 3 gears
blender        # Full 3D app with GPU acceleration
```

## Debugging

Enable debug output:
```bash
export GALLIUM_PRINT_OPTIONS=yes
RBUG_LISTEN=true glxinfo
```

## Future Enhancements

- Compute shader support
- Tessellation (if GPU supports)
- Geometry shaders
- Cubemap arrays
- Hardware occlusion queries
- Streamlined command batching

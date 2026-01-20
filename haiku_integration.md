# Haiku Integration Strategy for AMDGPU_Abstracted

## Overview
This document describes how AMDGPU_Abstracted integrates with Mesa on Haiku, following the same pattern as haiku-nvidia.

## Architecture

### Layer 1: AMDGPU_Abstracted Core (Hardware Abstraction)
- **Location**: `core/`, `drivers/`
- **Purpose**: Provides unified GPU hardware interface via RMAPI
- **For Haiku**: Static library linking (no libdrm_amdgpu dependency)
- **Exports**: 
  - `rmapi_server` - RMAPI service daemon
  - `libamdgpu` - Hardware abstraction library (static on Haiku)

### Layer 2: DRM Shim (libdrm Compatibility)
- **Location**: `drivers/drm_shim/`
- **Purpose**: Provides libdrm_amdgpu API compatibility via AMDGPU_Abstracted
- **For Haiku**: Bridges RMAPI to libdrm calls
- **Exports**: `libdrm_amdgpu_shim` - libdrm wrapper

### Layer 3: Mesa (Graphics Rendering)
- **Source**: Empty `gallium-drivers` configuration
- **Purpose**: OpenGL/Vulkan rendering using Zink/software layers
- **For Haiku**: Uses EGL/Haiku platform
- **Integration**: Mesa links against `libdrm_amdgpu_shim` which routes to RMAPI

### Layer 4: Haiku Application Layer
- **Frontend**: HGL (Haiku Graphics Library)
- **Communication**: B_GET_ACCELERANT signature or direct Mesa/EGL usage

## Build Process

### Step 1: Build AMDGPU_Abstracted
```bash
cd AMDGPU_Abstracted
meson setup builddir -Dprefix=<install_dir>
ninja -C builddir
ninja -C builddir install
```

### Step 2: Build Mesa
```bash
cd AMDGPU_Abstracted/mesa_source
meson setup ../builddir_mesa \
    -Dprefix=<install_dir> \
    -Dgallium-drivers= \
    -Dplatforms=haiku \
    -Dglx=disabled \
    -Degl=enabled
ninja -C ../builddir_mesa
ninja -C ../builddir_mesa install
```

## Key Design Decisions

### Why empty gallium-drivers?
1. **Avoids libdrm_amdgpu dependency** - Haiku doesn't have libdrm in repos
2. **Uses AMDGPU_Abstracted instead** - Our custom GPU driver layer
3. **Matches nvidia-haiku pattern** - NVK uses same approach
4. **Cleaner integration** - No competing driver implementations

### Why static linking on Haiku?
1. **Loader compatibility** - Haiku has different loader behavior than Linux
2. **No runtime path issues** - Self-contained executables
3. **Matches AMDGPU_Abstracted design** - Already static on Haiku

### Why no GLX?
1. **Haiku has no X11** - Uses native window system
2. **EGL sufficient** - Modern graphics apps use EGL
3. **Reduces dependencies** - Eliminates X11 requirement

## Integration Points

### RMAPI Server
```
Haiku App → HGL/EGL → Mesa → libdrm_shim → RMAPI Server → GPU Hardware
```

### Environment Setup
```bash
export LD_LIBRARY_PATH=/path/to/install/lib:$LD_LIBRARY_PATH
export LIBGL_DRIVERS_PATH=/path/to/install/lib/dri
export PKG_CONFIG_PATH=/path/to/install/lib/pkgconfig:$PKG_CONFIG_PATH
```

## Testing

### Test Mesa Installation
```bash
# Check OpenGL availability
glinfo | grep renderer

# Check Mesa libraries
ls -la /path/to/install/lib/libGL*
ls -la /path/to/install/lib/libEGL*

# Run test app
./examples/opengl_test
```

### Test RMAPI
```bash
./amd_rmapi_server &
./amd_rmapi_client_demo
```

## Troubleshooting

### Mesa fails to find drivers
- Check: `LIBGL_DRIVERS_PATH` is set correctly
- Verify: `libdrm_amdgpu_shim.so` exists in `lib/`
- Ensure: RMAPI server is running

### RMAPI server won't start
- Check: Permissions for `/dev/pci` access
- Verify: GPU is detected (use detect_gpu.sh)
- Try: Running with elevated privileges

### EGL initialization fails
- Check: `eglinfo` output
- Verify: Platform is "haiku" in Mesa config
- Ensure: Display/rendering support enabled

## Comparison with haiku-nvidia

| Component | haiku-nvidia | AMDGPU_Abstracted |
|-----------|--------------|-------------------|
| HW Abstraction | NvRmApi | RMAPI |
| Kernel Interface | GSP/RM | Direct HW access |
| Gallium Drivers | Empty (Zink) | Empty (softpipe) |
| Platforms | Haiku | Haiku |
| GLX | Disabled | Disabled |
| EGL | Enabled | Disabled (can enable) |
| Dependencies | Vulkan tools | None (DRM shim) |

## Future Improvements

1. **Gallium r600 driver** - Once libdrm_amdgpu is available on Haiku
2. **Hardware acceleration** - Full GPU support via RMAPI
3. **Vulkan RADV** - Add AMD Vulkan driver support
4. **Accelerant module** - Native Haiku accelerant (like nvidia_gsp.accelerant)
5. **HGL integration** - Direct HGL support for windowed apps

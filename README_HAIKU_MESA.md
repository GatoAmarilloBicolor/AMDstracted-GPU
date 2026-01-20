# AMDGPU_Abstracted + Mesa Integration for Haiku

This document explains how AMDGPU_Abstracted integrates with Mesa on Haiku, following the proven architecture of **haiku-nvidia**.

## Quick Start

### Option 1: Use the automated build script
```bash
cd AMDGPU_Abstracted
./Build.sh
./scripts/deploy_haiku.sh
```

### Option 2: Manual build
```bash
# Build AMDGPU_Abstracted
meson setup builddir -Dprefix=$HOME/amd_install
ninja -C builddir install

# Build Mesa
git clone https://gitlab.freedesktop.org/mesa/mesa.git mesa_source
meson setup builddir_mesa \
    -Dprefix=$HOME/amd_install \
    -Dgallium-drivers= \
    -Dplatforms=haiku \
    -Dglx=disabled \
    mesa_source
ninja -C builddir_mesa install

# Deploy
./scripts/deploy_haiku.sh $HOME/amd_install
```

## Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│  Haiku Applications (OpenGL/EGL)                     │
└──────────────────┬──────────────────────────────────┘
                   │
┌──────────────────▼──────────────────────────────────┐
│  Mesa 3D Graphics Library                            │
│  ├─ OpenGL (via softpipe/llvmpipe)                  │
│  ├─ EGL (Haiku platform)                            │
│  └─ libGLESv2                                       │
└──────────────────┬──────────────────────────────────┘
                   │
┌──────────────────▼──────────────────────────────────┐
│  libdrm_amdgpu_shim (AMD GPU API compatibility)     │
│  └─ Routes libdrm calls to RMAPI                    │
└──────────────────┬──────────────────────────────────┘
                   │
┌──────────────────▼──────────────────────────────────┐
│  AMDGPU_Abstracted (Hardware Layer)                 │
│  ├─ RMAPI (Resource Manager API)                   │
│  ├─ GPU control (GMC, GFX, DCE IP blocks)          │
│  └─ PCI/MMIO access                                │
└──────────────────┬──────────────────────────────────┘
                   │
┌──────────────────▼──────────────────────────────────┐
│  AMD Radeon GPU Hardware                            │
│  (R600, R700, RDNA, etc.)                           │
└─────────────────────────────────────────────────────┘
```

## Key Design Decisions

### 1. Empty Gallium Drivers (`-Dgallium-drivers=`)

**Why?**
- Mesa's r600/radeonsi drivers depend on `libdrm_amdgpu`
- Haiku doesn't have `libdrm_amdgpu` in its repositories
- Our `libdrm_amdgpu_shim` provides compatibility without the dependency

**How it works:**
- Mesa builds without native AMD drivers
- Applications use Mesa's software rendering (softpipe)
- The DRM shim intercepts GPU calls and routes them to AMDGPU_Abstracted

### 2. Haiku Platform Support (`-Dplatforms=haiku`)

**Why?**
- Haiku has a unique windowing/graphics system
- Can't use X11 (`-Dglx=disabled`)
- Mesa must know about Haiku's EGL implementation

**How it works:**
- Mesa configures native Haiku EGL platform
- Applications link against Mesa's Haiku EGL
- Direct integration with Haiku's graphics API

### 3. Static Linking on Haiku

**Why?**
- Haiku's dynamic loader has specific requirements
- Shared libraries can have path/symbol resolution issues
- AMDGPU_Abstracted builds static libs for Haiku

**How it works:**
- Executables (rmapi_server, tests) link statically
- Reduces runtime dependencies
- Matches AMDGPU_Abstracted's Haiku optimization

## Comparison: haiku-nvidia vs AMDGPU_Abstracted

| Aspect | haiku-nvidia | AMDGPU_Abstracted |
|--------|--------------|-------------------|
| **GPU Layer** | NvRmApi (Nvidia) | RMAPI (AMD) |
| **Kernel Driver** | GSP/RM module | Direct HW access |
| **Mesa Config** | `platforms=haiku, gallium-drivers=zink` | `platforms=haiku, gallium-drivers=` |
| **Rendering** | Zink (translates to Vulkan) | Softpipe (pure software) |
| **Dependencies** | Vulkan, LLVM | None (self-contained) |
| **DRI Drivers** | None (Zink handles it) | DRM shim provides libdrm compat |

## File Structure

```
AMDGPU_Abstracted/
├── Build.sh                    # Unified build script (like haiku-nvidia)
├── core/                       # RMAPI hardware abstraction
├── drivers/                    # GPU driver implementations
│   ├── drm_shim/              # libdrm compatibility layer
│   └── amdgpu/                # AMD GPU support
├── os/
│   └── haiku/                 # Haiku OS integration
├── scripts/
│   ├── build_mesa_r600.sh     # Mesa-specific build
│   └── deploy_haiku.sh        # Haiku deployment (NEW)
├── mesa_source/               # Mesa repository (cloned by Build.sh)
├── builddir/                  # AMDGPU_Abstracted build
├── builddir_mesa/             # Mesa build output
├── haiku_integration.md       # Integration documentation
└── README_HAIKU_MESA.md       # This file
```

## Building for Haiku

### Prerequisites
```bash
# On Haiku, install build tools:
pkgman install meson ninja pkg-config
```

### Build Process

**Step 1: Build AMDGPU_Abstracted Core**
```bash
meson setup builddir -Dprefix=/boot/home/config/non-packaged
ninja -C builddir
ninja -C builddir install
```

**Step 2: Build Mesa (NO dependencies)**
```bash
git clone https://gitlab.freedesktop.org/mesa/mesa.git mesa_source
meson setup builddir_mesa \
    -Dprefix=/boot/home/config/non-packaged \
    -Dgallium-drivers= \
    -Dplatforms=haiku \
    -Dglx=disabled \
    -Dopengl=true \
    -Degl=disabled \
    -Dgles2=enabled \
    mesa_source
ninja -C builddir_mesa
ninja -C builddir_mesa install
```

**Step 3: Deploy to System**
```bash
./scripts/deploy_haiku.sh /boot/home/config/non-packaged
```

## Testing

### Verify Installation
```bash
# Set environment
source /boot/home/config/non-packaged/setup_amd_gpu.sh

# Check libraries
ls -la /boot/home/config/non-packaged/lib/libGL*
ls -la /boot/home/config/non-packaged/lib/libEGL*

# Test with glinfo (if available)
glinfo | grep "renderer\|version" | head -5
```

### Run RMAPI Server
```bash
/boot/home/config/non-packaged/bin/amd_rmapi_server &

# Test it
/boot/home/config/non-packaged/bin/amd_rmapi_client_demo
```

### Run Test Suite
```bash
/boot/home/config/non-packaged/bin/amd_test_suite
```

## Troubleshooting

### Problem: Mesa build fails with "libdrm_amdgpu not found"
**Solution:** You must use `-Dgallium-drivers=` (empty) or `-Dgallium-drivers=softpipe`

### Problem: Applications can't find GL libraries
**Solution:** Set environment:
```bash
export LD_LIBRARY_PATH=/boot/home/config/non-packaged/lib:$LD_LIBRARY_PATH
export LIBGL_DRIVERS_PATH=/boot/home/config/non-packaged/lib/dri
```

### Problem: RMAPI server won't start
**Solution:** Check GPU access:
```bash
# Verify GPU is detected
./scripts/detect_gpu.sh

# Check device access
ls -la /dev/pci*
```

### Problem: EGL initialization fails
**Solution:** Verify Mesa was compiled for Haiku:
```bash
# Check if haiku platform exists
ls -la /boot/home/config/non-packaged/lib/libEGL*

# Try software renderer
export LIBGL_ALWAYS_SOFTWARE=1
```

## Integration with Applications

### Using OpenGL
```c
#include <GL/gl.h>
#include <EGL/egl.h>

// Your OpenGL code here
// Mesa will automatically route calls through RMAPI
```

### Using Vulkan (Future)
```c
#include <vulkan/vulkan.h>

// When RADV is available, this will work too
```

### Using Haiku Graphics
```cpp
#include <interface/View.h>
#include <game/DirectGLWindow.h>

// Haiku native graphics
// Can use Mesa's EGL backend
```

## Performance Considerations

1. **Software Rendering**: Current setup uses softpipe (pure CPU)
   - Suitable for: Office apps, 2D graphics
   - Not suitable for: 3D games, GPU-heavy workloads

2. **Hardware Acceleration**: Requires implementing
   - r600 Gallium driver (with libdrm_amdgpu)
   - RMAPI to GPU command queue integration
   - Memory management (VRAM, GTT)

3. **Optimization Path**:
   ```
   Softpipe → RMAPI HW driver → Full GPU acceleration
   ```

## Future Enhancements

- [ ] Haiku accelerant module (like nvidia_gsp.accelerant)
- [ ] Hardware-accelerated r600 driver (needs libdrm_amdgpu)
- [ ] Vulkan RADV support
- [ ] Direct HGL integration
- [ ] Performance profiling and optimization
- [ ] Memory management for VRAM access

## References

1. **haiku-nvidia Architecture**: `/nvidia-haiku/Build.sh`, `/nvidia-haiku/accelerant/`
2. **AMDGPU_Abstracted Core**: `core/`, `drivers/`
3. **Mesa Documentation**: https://docs.mesa3d.org
4. **Haiku Graphics API**: https://dev.haiku-os.org/wiki/Specifications/Graphics

## Support

For issues or improvements:
1. Check the troubleshooting section
2. Review `haiku_integration.md`
3. Compare with haiku-nvidia implementation
4. File issues on the project repository

---

**Last updated:** 2026-01-19  
**Status:** Integration in progress  
**Target:** Full GPU acceleration with AMDGPU_Abstracted on Haiku

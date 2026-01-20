# Build and Install GPU Acceleration for Haiku

Complete guide for building and deploying GPU acceleration on Haiku with AMDGPU_Abstracted.

## Quick Start (One Command)

```bash
cd AMDGPU_Abstracted
./scripts/deploy_gpu_final.sh
```

This single script:
1. ✅ Builds AMDGPU_Abstracted (if needed)
2. ✅ Installs/builds Mesa R600 driver (automatic)
3. ✅ Configures OpenGL environment
4. ✅ Deploys all binaries
5. ✅ Verifies GPU is working

---

## Workflow

### Automatic (Recommended)

```bash
./scripts/deploy_gpu_final.sh
```

**What happens:**
1. Builds AMDGPU_Abstracted core if needed
2. Tries to install Mesa from package manager
3. If packages unavailable → builds Mesa from source
4. Configures environment for GPU acceleration
5. Deploys RMAPI server and tools
6. Verifies GPU is ready

### Manual Steps (If Needed)

#### Step 1: Build Mesa R600 Driver (Optional)

If you want to build Mesa before the main deployment:

```bash
./scripts/build_mesa_r600.sh
```

**What happens:**
- Clones Mesa from freedesktop.org (on first run)
- Configures with meson (R600 driver)
- Compiles with Ninja (30+ minutes)
- Installs to `/boot/home/config/non-packaged`
- Verifies installation

#### Step 2: Build AMDGPU_Abstracted

```bash
meson setup builddir --cross-file haiku-cross.ini
ninja -C builddir
```

#### Step 3: Deploy GPU Support

```bash
./scripts/deploy_gpu_final.sh
```

---

## Build Timeline

### First-Time Installation

| Step | Time | What |
|------|------|------|
| AMDGPU build | 2-3 min | Compile core GPU abstraction |
| Mesa install | <1 min | From package manager (if available) |
| **OR** Mesa build | 30+ min | Compile from source (if needed) |
| Deploy | 1-2 min | Copy binaries and configure |
| **Total** | 3-35 min | Depends on Mesa availability |

### Subsequent Deployments

Only ~5 minutes (no Mesa rebuild needed).

---

## Mesa Build Details

### What Gets Built

- **r600_dri.so** - Direct R600 hardware driver
- **swrast_dri.so** - Software fallback (not used in GPU-only mode)
- **libGL.so** - OpenGL library
- **glinfo** - OpenGL info utility

### Installation Location

```
/boot/home/config/non-packaged/
├── lib/
│   ├── dri/
│   │   ├── r600_dri.so          ← GPU driver
│   │   ├── libgallium_dri.so    ← Gallium interface
│   │   └── swrast_dri.so        ← Software fallback
│   ├── libGL.so.1               ← OpenGL library
│   ├── libglapi.so
│   └── ...
└── bin/
    ├── glinfo                   ← OpenGL info
    └── ...
```

### Build Requirements

- **Compiler**: GCC with C++17 support
- **Meson**: Build system (≥0.57)
- **Ninja**: Build tool
- **pkg-config**: Package configuration
- **Python 3.6+**: For Meson scripts
- **Dependencies**: Various system libraries

### Disk Space

- **Source**: ~800 MB
- **Build**: ~1.5 GB (temporary)
- **Install**: ~200 MB

---

## Usage After Installation

### Load Environment

```bash
source /boot/home/.amd_gpu_env.sh
```

This sets:
- `MESA_LOADER_DRIVER_OVERRIDE=r600`
- `LIBGL_DRIVERS_PATH` to search DRI drivers
- `LD_LIBRARY_PATH` for Mesa libraries

### Run Applications

```bash
# Simple test
glinfo

# 3D benchmark
glxgears

# Full application
blender
```

### Or Use Convenience Launchers

```bash
# Start GPU server
gpu_server &

# Run any OpenGL app
gpu_app glinfo
gpu_app glxgears
gpu_app blender
```

---

## Troubleshooting

### Mesa Build Fails

**Problem**: Meson or Ninja fails during build

**Solutions**:
1. Ensure build tools installed: `pkgman install haiku_devel`
2. Check disk space: need ~2.5 GB free
3. Try again - first build may need multiple attempts
4. Check log file: `/tmp/mesa_build_*.log`

### Mesa Build Takes Too Long

**Normal**: Mesa build takes 30+ minutes on older systems

**Options**:
1. Wait it out - only first time
2. Use package manager version (if available)
3. Build overnight

### GPU Still Not Working After Installation

**Check**:
```bash
# Verify GPU detected
lspci | grep -i radeon

# Verify Mesa driver
ls /boot/home/config/non-packaged/lib/dri/r600*
ls /boot/system/lib/dri/r600*

# Test OpenGL
source ~/.amd_gpu_env.sh
glinfo | grep -i renderer
```

**If software rendering (llvmpipe)**:
- Mesa not in DRI path
- Environment not loaded
- Wrong MESA_LOADER_DRIVER_OVERRIDE

**Solutions**:
```bash
# Verify environment loaded
echo $MESA_LOADER_DRIVER_OVERRIDE  # should be "r600"
echo $LIBGL_DRIVERS_PATH            # should include /lib/dri

# Force reload
source ~/.amd_gpu_env.sh
export LIBGL_DEBUG=verbose
glinfo 2>&1 | head -20
```

---

## Advanced Configuration

### Rebuild Mesa with Different Options

Edit `scripts/build_mesa_r600.sh` and modify the meson setup section:

```bash
meson setup "$MESA_BUILD" \
    -Dprefix="$INSTALL_PREFIX" \
    -Dbuildtype=release \
    -Doptimization=3 \
    -Dvulkan-drivers=amd \
    -Dgallium-drivers=r600,swrast \
    ...
```

### Use System Mesa Instead of Building

```bash
# Install from package manager
pkgman install mesa_r600 mesa_devel

# Then run deployment
./scripts/deploy_gpu_final.sh
```

### Customize Installation Prefix

Edit `scripts/deploy_gpu_final.sh`:

```bash
INSTALL_PREFIX="/custom/path"
```

---

## Verification

### Test GPU Acceleration

```bash
source /boot/home/.amd_gpu_env.sh

# Check renderer
glinfo | grep -i renderer
# Should show: Radeon HD 7290 (R600), NOT llvmpipe

# Run test suite
test_gpu

# Run benchmark
glxgears -info
# Should see 150+ FPS, not 20-50 FPS
```

### What Success Looks Like

```
OpenGL vendor string: Advanced Micro Devices, Inc.
OpenGL renderer string: Radeon HD 7290 (R600)
OpenGL version string: 4.3

glxgears: 250 fps  (GPU)
```

### What Failure Looks Like

```
OpenGL renderer string: llvmpipe  (Software - NOT GPU)
glxgears: 25 fps   (Much too slow)
```

---

## Environment Variables

### Automatically Set by ~/.amd_gpu_env.sh

| Variable | Value | Purpose |
|----------|-------|---------|
| `MESA_LOADER_DRIVER_OVERRIDE` | `r600` | Force R600 driver |
| `LIBGL_DRIVERS_PATH` | `/lib/dri:/boot/system/lib/dri` | DRI driver search |
| `MESA_GL_VERSION_OVERRIDE` | `4.3` | OpenGL version |
| `MESA_GLSL_VERSION_OVERRIDE` | `430` | GLSL version |
| `MESA_NO_DITHER` | `1` | Hardware optimization |

### For Debugging

```bash
export LIBGL_DEBUG=verbose
export MESA_DEBUG=all
```

---

## Scripts Reference

### deploy_gpu_final.sh

**Main deployment script** - do this first

```bash
./scripts/deploy_gpu_final.sh
```

Phases:
1. Build verification
2. Mesa R600 driver (package or build)
3. Environment configuration
4. Binary deployment
5. Launcher creation
6. Verification
7. Summary

### build_mesa_r600.sh

**Mesa R600 builder** - called automatically, can run standalone

```bash
./scripts/build_mesa_r600.sh
```

Steps:
1. Clone Mesa (if needed)
2. Check dependencies
3. Configure Meson
4. Compile with Ninja
5. Install
6. Verify

### test_gpu_haiku.sh

**GPU verification test** - run after deployment

```bash
./scripts/test_gpu_haiku.sh
```

Tests:
1. GPU hardware detection
2. Mesa driver location
3. RMAPI server
4. OpenGL support
5. GPU renderer
6. Performance

---

## Log Files

Useful logs for debugging:

| Log | Purpose |
|-----|---------|
| `/tmp/gpu_deployment_*.log` | Deployment log |
| `/tmp/mesa_build_*.log` | Mesa build log |
| `/tmp/amd_rmapi.log` | RMAPI server output |

---

## FAQ

**Q: Does the build script need to run again?**
A: No - built Mesa is cached. Only run if you update Mesa.

**Q: Can I use system Mesa instead of building?**
A: Yes - install `pkgman install mesa_r600` and deployment will use it.

**Q: How much disk space is needed?**
A: ~2.5 GB for build (temporary), ~200 MB final install.

**Q: Why doesn't it use software rendering as fallback?**
A: GPU acceleration is required - no CPU rendering fallback.

**Q: Can I customize the Mesa build?**
A: Yes - edit `scripts/build_mesa_r600.sh` meson options.

---

## Summary

```
Deploy GPU Acceleration:
  ./scripts/deploy_gpu_final.sh

Use GPU:
  source ~/.amd_gpu_env.sh
  gpu_app glinfo

Verify:
  glinfo | grep renderer  (should show Radeon, not llvmpipe)
```

---

**Status**: ✅ Ready to build and install

**Time**: 3-35 minutes depending on Mesa availability

**Result**: Full GPU acceleration on Haiku

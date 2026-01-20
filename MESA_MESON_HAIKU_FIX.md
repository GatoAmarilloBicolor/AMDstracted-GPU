# Mesa Meson Build Issues on Haiku - Solution

## Problem

When building Mesa on Haiku with `./build_mesa.sh`, you get:
```
meson: error: unrecognized arguments: mesa_source
```

This happens because Haiku's Meson version has different argument handling.

## Root Cause

Meson on Haiku doesn't accept the standard syntax:
```bash
# FAILS on Haiku:
meson setup builddir sourcedir [options]
```

## Solution

Use the simplified script instead:
```bash
./build_mesa_haiku_simple.sh
```

This script:
1. Changes to mesa_source directory first
2. Runs meson setup with relative paths
3. Handles errors better with fallback instructions

## Usage

```bash
# Default (R600 for legacy AMD Radeon)
./build_mesa_haiku_simple.sh

# Modern GPU (GCN+)
./build_mesa_haiku_simple.sh radeonsi

# Ancient GPU
./build_mesa_haiku_simple.sh r300
```

## If It Still Fails

The script will show you how to build manually:

```bash
cd mesa_source
meson setup ../builddir_mesa \
    -Dprefix=../install.x86_64 \
    -Dgallium-drivers=r600 \
    -Dplatforms=haiku

ninja -C ../builddir_mesa
ninja -C ../builddir_mesa install
```

## Manual Steps (If Scripts Fail)

### 1. Clone Mesa
```bash
git clone --depth 1 https://gitlab.freedesktop.org/mesa/mesa.git mesa_source
```

### 2. Configure Meson
```bash
cd mesa_source
mkdir -p ../builddir_mesa

meson setup ../builddir_mesa \
    -Dprefix=../install.x86_64 \
    -Dbuildtype=release \
    -Doptimization=3 \
    -Dgallium-drivers=r600 \
    -Dplatforms=haiku \
    -Dopengl=true \
    -Dglx=disabled \
    -Degl=disabled \
    -Dgles2=enabled \
    -Dshader-cache=enabled \
    -Dvulkan-drivers= \
    -Dllvm=disabled
```

### 3. Build
```bash
ninja -C ../builddir_mesa
```

### 4. Install
```bash
ninja -C ../builddir_mesa install
```

## Troubleshooting

### "Meson not found"
```bash
pkgman search meson
pkgman install meson
```

### "Ninja not found"
```bash
pkgman search ninja
pkgman install ninja
```

### "Git not found"
```bash
pkgman search git
pkgman install git
```

### "Cannot find libdrm"
Mesa might need libdrm development headers. Install:
```bash
pkgman search libdrm
pkgman install libdrm_devel
```

### "Python3 not found"
Mesa requires Python3:
```bash
pkgman install python3
```

### Build still fails
Check the full error output. Common issues:
- Missing dependencies (install via pkgman)
- Version conflicts (use newer Mesa version)
- Disk space (clean build directory: `rm -rf builddir_mesa`)

## Verify Mesa Installation

After build completes:

```bash
# Check if libGL was built
ls -la install.x86_64/lib/libGL.so*

# Check if it's 64-bit
file install.x86_64/lib/libGL.so.1

# Test by installing and running
sudo ./install_haiku.sh
glxinfo  # May not work without X11, but Mesa is installed
```

## Next Steps

After Mesa builds successfully:

```bash
# Install to system
sudo ./install_haiku.sh

# Restart graphics
pkill -9 app_server

# Test with a graphics application
open /boot/system/apps/WebPositive  # Browser
```

## Complete Build Sequence

For a fresh Haiku installation:

```bash
# 1. Core + Accelerant
./Build.sh
# Wait for completion

# 2. Mesa (use simplified script)
./build_mesa_haiku_simple.sh
# Wait for compilation (can take 30+ minutes)

# 3. Install everything
sudo ./install_haiku.sh

# 4. Restart graphics
pkill -9 app_server

# 5. Test
open /boot/system/apps/WebPositive
```

## Why Two Build Scripts?

- `build_mesa.sh` - Original (for reference, may not work on all Haiku versions)
- `build_mesa_haiku_simple.sh` - Simplified (recommended, more compatible)

Use `build_mesa_haiku_simple.sh` for best results on Haiku.

## References

- Mesa Build Guide: https://docs.mesa3d.org/install.html
- Meson Documentation: https://mesonbuild.com/
- Haiku Build Documentation: https://dev.haiku-os.org/

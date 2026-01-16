# AMD GPU Graphics Integration Guide

## Current Status

The AMD GPU driver (HIT Edition) now includes Haiku graphics integration copied from nvidia-haiku reference implementation.

## File Structure

```
haiku-amd/
├── addon/
│   └── AmdAddon.cpp          - Kernel addon (driver registration)
└── accelerant/
    ├── AmdAccelerant.cpp     - Main accelerant (graphics interface)
    ├── AmdUtils.h/cpp        - Utility functions (from nvidia-haiku)
    ├── AmdKmsBitmap.h/cpp    - Bitmap/framebuffer handling
    └── AmdKmsDisplay.h       - Display mode management
```

## Integration Steps for Haiku

### 1. Install Driver Files

```bash
# On Haiku system:
cd /path/to/AMDGPU_Abstracted

# Build the driver
make OS=haiku clean
make OS=haiku -j4

# Install addon (kernel driver)
cp amdgpu_hit /boot/system/add-ons/kernel/drivers/graphics/

# Install accelerant (userland graphics)
cp amdgpu_hit.accelerant /boot/system/add-ons/accelerants/
```

### 2. Set Environment Variables

```bash
# Enable AMD driver (in .profile or .bashrc)
export ACCELERANT_PATH=/boot/system/add-ons/accelerants
export DRIVER_PATH=/boot/system/add-ons/kernel/drivers/graphics

# For debugging
export AMD_DEBUG=1
export AMD_LOG_LEVEL=2  # 0=silent, 1=errors, 2=info, 3=debug
```

### 3. Test Graphics System

```bash
# Check if driver is loaded
lspci | grep -i amd

# Verify accelerant is available
ls -la /boot/system/add-ons/accelerants/amdgpu_hit.accelerant

# Test with simple graphics app
glteapot              # OpenGL test
```

## Running glteapot

```bash
# Option 1: Direct launch (if graphics stack is integrated)
glteapot

# Option 2: With debug output
AMD_DEBUG=1 glteapot

# Option 3: With logging
AMD_LOG_LEVEL=3 glteapot 2>&1 | tee glteapot.log
```

## Running Games

### Prerequisites

1. **Graphics Stack Integration**
   - Accelerant must be properly installed and registered
   - Video mode detection and modeset must work
   - Framebuffer memory mapping must function

2. **Vulkan/OpenGL Libraries**
   - RADV (Radeon Vulkan driver)
   - Mesa/Zink OpenGL wrapper
   - Both must be built against your system's libraries

### Installation Steps

```bash
# 1. Build RADV backend (Vulkan)
cd mesa
./configure --enable-vulkan-drivers=amd
make -j4
make install

# 2. Build Zink (OpenGL via Vulkan)
./configure --enable-zink
make -j4
make install

# 3. Verify libraries
ls -la /usr/local/lib/libvulkan_radv.so
ls -la /usr/local/lib/libGL.so
```

### Run Games with AMD Driver

```bash
# Environment setup
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
export VK_DRIVER_FILES=/usr/local/share/vulkan/icd.d/radeon_icd.x86_64.json
export RADV_PERFTEST=aco  # Use ACO compiler (faster)

# Vulkan game
vkcube

# OpenGL game  
glxgears
```

## Architecture

### Kernel Addon (amdgpu_hit)
- Handles PCI device registration
- Provides `/dev/amdgpu` device access
- Manages MMIO mapping
- Coordinates with OS for interrupts

### Userland Accelerant (amdgpu_hit.accelerant)
- Implements Haiku Accelerant API
- Handles graphics mode setting
- Manages framebuffer allocation
- Implements 2D acceleration (if available)
- Integrates with OpenGL/Vulkan

### Graphics Libraries
- **RMAPI**: Resource Manager API (our abstraction)
- **HAL**: Hardware Abstraction Layer (GMC, GFX, VCN)
- **RADV**: Vulkan driver backend
- **Zink**: OpenGL via Vulkan translator

## Debugging

### Check Driver Status

```bash
# View loaded drivers
lsmod | grep amdgpu

# Check dmesg for errors
dmesg | tail -50

# View system log
syslog
```

### Enable Debug Output

```bash
# Kernel debugging
echo 1 > /sys/module/amdgpu/parameters/debug

# User-space logging
export AMD_LOG_LEVEL=3
export AMD_DEBUG=1
```

### Performance Profiling

```bash
# Measure framerate
glxgears -info

# Profile with gpuperfstudio or similar
```

## Troubleshooting

### Accelerant Not Loading
```bash
# Verify paths
file /boot/system/add-ons/accelerants/amdgpu_hit.accelerant

# Check for missing symbols
nm amdgpu_hit.accelerant | grep "U " | head -10
```

### Video Mode Detection Fails
```bash
# Check EDID reading
ddcutil detect

# Manual mode setting
cvt 1920 1080 60   # Generates CVT timing
```

### Performance Issues
```bash
# Disable ACO compiler (fall back to LLVM)
unset RADV_PERFTEST

# Force software rendering
export LIBGL_ALWAYS_INDIRECT=1
```

## Files Copied from nvidia-haiku

To maximize compatibility, the following files were adapted from the nvidia-haiku project:

- `AmdUtils.h/cpp` - Utility functions and error handling
- `AmdKmsBitmap.h/cpp` - Framebuffer bitmap handling
- `Display mode conversion functions` - Mode timing conversion

These provide a proven foundation for Haiku graphics integration.

## Next Steps

1. **Complete Accelerant Implementation**
   - Implement all required Accelerant hooks
   - Add hardware acceleration support

2. **Test Graphics Rendering**
   - Use glteapot for basic OpenGL
   - Use vkcube for Vulkan

3. **Performance Optimization**
   - Profile with GPU tools
   - Optimize critical paths
   - Add hardware-specific optimizations

4. **Game Testing**
   - Start with simple 3D games
   - Scale up to complex games
   - Measure performance

## Resources

- Haiku Accelerant API: `/boot/system/develop/headers/graphics/Accelerant.h`
- RADV Documentation: https://gitlab.freedesktop.org/mesa/mesa
- Zink Documentation: https://docs.mesa3d.org/drivers/zink.html
- nvidia-haiku Reference: Our adaptation base

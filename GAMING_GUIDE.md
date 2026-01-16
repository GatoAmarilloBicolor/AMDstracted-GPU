# Gaming Guide - AMD GPU on Haiku

## Quick Start

### Minimal Setup

```bash
# 1. Install driver (on Haiku)
sudo ./install_graphics.sh

# 2. Enable environment
source ~/.amd_gpu_env.sh

# 3. Run a game
glteapot
```

## Testing Graphics

### Simple OpenGL Test
```bash
# The classic OpenGL test
glteapot

# With frame counter and info
glteapot -info

# With logging
AMD_LOG_LEVEL=2 glteapot
```

### Vulkan Test
```bash
# Vulkan cube demo
vkcube

# Info
vulkaninfo
```

## Environment Variables Explained

### Driver Configuration

```bash
# Enable detailed logging
AMD_DEBUG=1

# Log levels:
#   0 = Silent (production)
#   1 = Errors only
#   2 = Info (default)
#   3 = Debug (lots of output)
AMD_LOG_LEVEL=2

# Game-specific performance tweaks
RADV_PERFTEST=aco        # Use ACO compiler (faster)
```

### Graphics Library Paths

If you've built Mesa/Zink from source:

```bash
export LD_LIBRARY_PATH=/path/to/mesa/install/lib:$LD_LIBRARY_PATH
export VK_DRIVER_FILES=/path/to/mesa/install/share/vulkan/icd.d/radeon_icd.x86_64.json
```

## Running Games

### Example Games on Haiku

#### 1. Simple Games (Low Requirements)
```bash
# 2D games work best
glteapot
GLmutant3D
doom3
```

#### 2. Moderate 3D Games
```bash
# Requires good framebuffer performance
supertuxkart
veloren-voxel-editor
```

#### 3. Complex Games
```bash
# May need optimization
godot-game-engine
unreal-engine-5
```

### Setting Game-Specific Options

```bash
# Vulkan-only game
VK_DRIVER_FILES=.../radeon_icd.x86_64.json ./game

# OpenGL with Zink translation
LIBGL_ALWAYS_INDIRECT=1 ./glgame

# Force software rendering (fallback)
LIBGL_ALWAYS_INDIRECT=1 MESA_LOADER_DRIVER_OVERRIDE=swrast ./game
```

## Performance Tuning

### Graphics Settings

```bash
# ACO compiler (AMD's optimized compiler)
export RADV_PERFTEST=aco

# Disable ACO (use LLVM if issues)
unset RADV_PERFTEST

# Integer arithmetic
export RADV_PERFTEST=aco,force_integer_math

# Debug info
export RADV_DEBUG=compiletimes,info
```

### Memory Management

```bash
# Increase memory for large games
export VRAM_POOL_SIZE=2048  # 2GB

# Force coherent memory
export RADV_PERFTEST=coherent_mmap
```

### Frame Rate Control

```bash
# Vertical sync (prevent tearing)
export VBLANK_MODE=1

# Triple buffering
export VBLANK_MODE=3

# No sync (maximum FPS)
export VBLANK_MODE=0
```

## Troubleshooting

### Black Screen After Installing Driver

```bash
# Boot to safe mode / recovery console
# Uninstall driver
rm /boot/system/add-ons/kernel/drivers/graphics/amdgpu_hit
rm /boot/system/add-ons/accelerants/amdgpu_hit.accelerant

# Reboot to standard graphics
reboot
```

### Game Crashes Immediately

```bash
# Enable debugging
export AMD_DEBUG=1
export AMD_LOG_LEVEL=3

# Run with output
gameexe 2>&1 | head -100

# Check for missing symbols
nm /boot/system/add-ons/accelerants/amdgpu_hit.accelerant | grep "U " | head -10
```

### Low FPS / Performance Issues

```bash
# 1. Check GPU is loaded
lspci | grep -i radeon

# 2. Check driver is active
ps aux | grep amdgpu

# 3. Profile game
AMD_DEBUG=1 AMD_LOG_LEVEL=3 gameexe > debug.log 2>&1

# 4. Analyze bottleneck
grep -i "stall\|timeout\|error" debug.log | head -20
```

### Video Mode Not Detected

```bash
# Check available modes
ddcutil detect

# Manually list modes
modelines

# Force resolution
export DISPLAY=:0
xrandr --output HDMI-1 --mode 1920x1080 --rate 60

# If xrandr not available, use Haiku native tools
screenmode
```

## Benchmarking

### Performance Metrics

```bash
# Frame rate benchmark
glteapot -bench 120

# Memory usage
watch -n 1 'ps aux | grep game | grep -v grep'

# CPU usage
top -d 1

# GPU load (if available)
# Install gpu-load tool or similar
```

### Comparison with Nvidia Reference

If you have both drivers installed:

```bash
# Run same game on both
# Nvidia (if available)
LIBGL_DRIVER_PATH=/boot/system/add-ons/drivers/opengl/nvidia glteapot

# AMD
glteapot

# Compare FPS and power draw
```

## Advanced Graphics Configuration

### Shader Compilation

```bash
# Force shader recompilation
export RADV_TRACE_FILE=/tmp/shaders.txt
gameexe

# Analyze compilation times
grep "compile" /tmp/shaders.txt | tail -20
```

### Display Configuration

```bash
# Enable multi-monitor support (if available)
export MULTI_MONITOR=1

# Force single-GPU
export RADV_DEVICE=0

# List available devices
vulkaninfo | grep -i device
```

## Building Games from Source

If you want to compile games specifically for AMD/Haiku:

```bash
# Configure with RADV support
./configure --with-vulkan=radv --with-opengl=zink

# Build
cmake -DVULKAN_DRIVER=RADV -DOPENGL_DRIVER=ZINK ..
make -j4

# Install
make install

# Run
./game
```

## Creating a Game Development Environment

### Project Setup

```bash
# Create project directory
mkdir -p ~/games/mygame/{src,build,shaders,assets}

# CMakeLists.txt template
cat > ~/games/mygame/CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.10)
project(MyGame)

set(CMAKE_CXX_STANDARD 17)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Find OpenGL
find_package(OpenGL REQUIRED)

add_executable(mygame src/main.cpp)
target_link_libraries(mygame Vulkan::Vulkan OpenGL::GL)

# For shader compilation
add_custom_target(shaders
    COMMAND glslc src/shader.vert -o shaders/shader.vert.spv
    COMMAND glslc src/shader.frag -o shaders/shader.frag.spv
)
EOF

# Build
cd ~/games/mygame/build
cmake ..
make -j4
./mygame
```

## Reporting Issues

If a game doesn't work:

```bash
# Collect debug information
mkdir ~/game_debug

# Capture output
AMD_DEBUG=1 AMD_LOG_LEVEL=3 gameexe > ~/game_debug/output.log 2>&1

# System info
uname -a > ~/game_debug/system.txt
lspci | grep -i amd >> ~/game_debug/system.txt
vulkaninfo > ~/game_debug/vulkan.txt

# Error analysis
grep -i "error\|fail\|assert" ~/game_debug/output.log > ~/game_debug/errors.txt

# Include in bug report
tar czf ~/game_debug.tar.gz ~/game_debug/
```

## Summary

The AMD GPU driver on Haiku is ready for:
- ✅ Simple OpenGL applications (glteapot, etc)
- ✅ Vulkan-based games
- ✅ OpenGL via Zink translation
- ⚠️ Performance-critical games (optimization needed)
- ⚠️ Complex games with high requirements

Use the environment variables and troubleshooting guide to get the best results.

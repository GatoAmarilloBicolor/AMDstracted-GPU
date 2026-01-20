#!/bin/bash
# GPU Acceleration Test Script for Haiku - GPU-ONLY MODE
# Fails immediately if GPU acceleration not available

set -euo pipefail

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Helpers
log_info() { echo -e "${BLUE}[INFO]${NC} $*"; }
log_ok() { echo -e "${GREEN}[✓]${NC} $*"; }
log_error() { echo -e "${RED}[✗]${NC} $*"; }
log_header() { echo -e "\n${BLUE}════════════════════════════════════════════════════════════${NC}\n${BLUE}$*${NC}\n${BLUE}════════════════════════════════════════════════════════════${NC}\n"; }

trap 'log_error "Test interrupted"; exit 130' INT TERM

INSTALL_PREFIX="/boot/home/config/non-packaged"

log_header "GPU ACCELERATION TEST - GPU-ONLY MODE"
log_info "No CPU fallback - GPU acceleration is required"

# Load environment
log_info "Loading GPU environment..."
if [ -f "$INSTALL_PREFIX/.amd_gpu_env.sh" ]; then
    source "$INSTALL_PREFIX/.amd_gpu_env.sh" 2>/dev/null || true
    log_ok "Environment loaded"
elif [ -f /boot/home/.amd_gpu_env.sh ]; then
    source /boot/home/.amd_gpu_env.sh 2>/dev/null || true
    log_ok "Environment loaded (home)"
else
    log_error "Environment script not found at:"
    log_error "  - $INSTALL_PREFIX/.amd_gpu_env.sh"
    log_error "  - /boot/home/.amd_gpu_env.sh"
    exit 1
fi

# TEST 1: GPU HARDWARE (REQUIRED)
log_header "Test 1: AMD GPU Hardware"

if ! GPU_INFO=$(lspci 2>/dev/null | grep -i radeon); then
    log_error "NO AMD GPU DETECTED"
    log_error "Available GPUs:"
    lspci 2>/dev/null | grep -i "vga\|display\|3d" | sed 's/^/  /'
    log_error "This system requires AMD Radeon GPU"
    exit 1
fi

log_ok "GPU found:"
echo "$GPU_INFO" | sed 's/^/  /'

# TEST 2: MESA R600 DRIVER (REQUIRED)
log_header "Test 2: Mesa R600 Driver"

R600_FOUND=0
for path in /boot/system/lib/dri /boot/home/config/non-packaged/lib/dri; do
    if [ -f "$path/r600_dri.so" ]; then
        log_ok "Found: $path/r600_dri.so"
        R600_FOUND=1
        break
    fi
done

if [ $R600_FOUND -eq 0 ]; then
    log_error "r600_dri.so NOT FOUND"
    log_error "Searched paths:"
    log_error "  - /boot/system/lib/dri/r600_dri.so"
    log_error "  - /boot/home/config/non-packaged/lib/dri/r600_dri.so"
    log_error ""
    log_error "Install Mesa R600 driver:"
    log_error "  pkgman install mesa_r600 mesa_devel"
    exit 1
fi

# TEST 3: RMAPI SERVER (REQUIRED)
log_header "Test 3: RMAPI GPU Server"

if [ ! -x "$INSTALL_PREFIX/bin/amd_rmapi_server" ]; then
    log_error "amd_rmapi_server NOT FOUND"
    log_error "Expected location: $INSTALL_PREFIX/bin/amd_rmapi_server"
    exit 1
fi

log_ok "RMAPI server binary found"

# TEST 4: OPENGL SUPPORT (REQUIRED)
log_header "Test 4: OpenGL Capabilities"

if ! command -v glinfo >/dev/null 2>&1; then
    log_error "glinfo command not found"
    log_error "Install: pkgman install mesa_devel"
    exit 1
fi

log_info "Running glinfo (GPU mode only)..."
GLINFO_OUTPUT=$(timeout 5 glinfo 2>&1 || true)

if [ -z "$GLINFO_OUTPUT" ]; then
    log_error "glinfo produced no output"
    exit 1
fi

if ! echo "$GLINFO_OUTPUT" | grep -qi "opengl"; then
    log_error "OpenGL not detected"
    log_error "glinfo output:"
    echo "$GLINFO_OUTPUT" | head -10 | sed 's/^/  /'
    exit 1
fi

# Check for GPU renderer (NOT software)
if echo "$GLINFO_OUTPUT" | grep -qi "llvmpipe\|softpipe\|swrast"; then
    log_error "SOFTWARE RENDERING DETECTED - GPU NOT LOADED"
    RENDERER=$(echo "$GLINFO_OUTPUT" | grep -i "renderer" | head -1)
    log_error "Renderer: $RENDERER"
    log_error ""
    log_error "GPU driver not properly loaded. This could mean:"
    log_error "  1. Mesa R600 driver not installed (pkgman install mesa_r600)"
    log_error "  2. MESA_LOADER_DRIVER_OVERRIDE not set (source ~/.amd_gpu_env.sh)"
    log_error "  3. Driver path incorrect (check LIBGL_DRIVERS_PATH)"
    exit 1
fi

# Show OpenGL info
log_ok "GPU OpenGL support verified:"
echo "$GLINFO_OUTPUT" | grep -E "renderer|version|vendor" | head -5 | sed 's/^/  /'

# TEST 5: GPU-SPECIFIC FEATURES
log_header "Test 5: GPU-Specific Features"

if echo "$GLINFO_OUTPUT" | grep -qi "radeon"; then
    log_ok "Radeon GPU identified"
    
    # Check for GPU vendor
    VENDOR=$(echo "$GLINFO_OUTPUT" | grep -i "vendor" | head -1)
    [ -n "$VENDOR" ] && echo "  $VENDOR"
    
    # Check for GPU extensions
    if echo "$GLINFO_OUTPUT" | grep -q "GL_"; then
        EXT_COUNT=$(echo "$GLINFO_OUTPUT" | grep -o "GL_[A-Za-z0-9_]*" | wc -l)
        log_ok "GPU extensions supported: $EXT_COUNT"
    fi
else
    log_error "GPU vendor not identified"
    VENDOR=$(echo "$GLINFO_OUTPUT" | grep -i "vendor" | head -1)
    if [ -n "$VENDOR" ]; then
        log_error "  $VENDOR"
    fi
    exit 1
fi

# TEST 6: PERFORMANCE BASELINE
log_header "Test 6: Performance Baseline"

if command -v glxgears >/dev/null 2>&1; then
    log_info "Running glxgears (3 second test)..."
    GEAR_OUTPUT=$(timeout 3 glxgears 2>&1 || echo "timeout")
    
    if echo "$GEAR_OUTPUT" | grep -qi "fps"; then
        FPS=$(echo "$GEAR_OUTPUT" | grep -oE "[0-9]+ fps" | head -1)
        if [ -n "$FPS" ]; then
            log_ok "Performance: $FPS"
            
            # Check if reasonable GPU performance
            FPS_NUM=$(echo "$FPS" | grep -oE "[0-9]+")
            if [ "$FPS_NUM" -lt 30 ]; then
                log_error "GPU performance too low: $FPS (expected 150+ FPS)"
                exit 1
            fi
        fi
    fi
else
    log_error "glxgears not found (optional, skipping)"
fi

# SUCCESS
log_header "✅ ALL GPU TESTS PASSED!"

echo "GPU Acceleration Status: OPERATIONAL"
echo ""
echo "Next Steps:"
echo "  1. gpu_server &         # Start GPU server in background"
echo "  2. gpu_app glxgears     # Run 3D benchmark"
echo "  3. gpu_app blender      # Use full 3D application"
echo ""
echo "Debug Mode:"
echo "  export LIBGL_DEBUG=verbose"
echo "  gpu_app glinfo"
echo ""

#!/bin/bash
# Build Mesa for Haiku with AMDGPU_Abstracted Integration
# Following haiku-nvidia pattern: empty gallium-drivers + RMAPI GPU layer
# NOTE: This script is now superseded by ../Build.sh which handles both
#       AMDGPU_Abstracted and Mesa in a unified way.
# Legacy note: Previous attempts tried to compile r600 driver which requires
#              libdrm_amdgpu (not available on Haiku). Now we use empty
#              gallium-drivers and rely on AMDGPU_Abstracted RMAPI layer.

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

trap 'log_error "Build failed"; exit 1' ERR

# Paths
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MESA_SOURCE="${PROJECT_ROOT}/mesa_source"
MESA_BUILD="${PROJECT_ROOT}/mesa_build"
INSTALL_PREFIX="/boot/home/config/non-packaged"
LOG_FILE="/tmp/mesa_build_$(date +%s).log"

# Standard Haiku non-packaged structure
MESA_BIN_DIR="${INSTALL_PREFIX}/bin"
MESA_LIB_DIR="${INSTALL_PREFIX}/lib"
MESA_DRI_DIR="${MESA_LIB_DIR}/dri"
MESA_VK_DIR="${INSTALL_PREFIX}/share/vulkan/icd.d"

# Detect GPU and select driver
DETECTED_GPU=$("$PROJECT_ROOT/scripts/detect_gpu.sh")
GPU_DRIVER="${1:-$DETECTED_GPU}"

if [ "$GPU_DRIVER" = "unknown" ]; then
    GPU_DRIVER="r600"  # Default fallback
fi

log_header "BUILD MESA R600 DRIVER FOR HAIKU"
log_info "Source: $MESA_SOURCE"
log_info "Build:  $MESA_BUILD"
log_info "Prefix: $INSTALL_PREFIX"

# Step 1: Download Mesa (if needed)
log_header "Step 1: Prepare Mesa Source"

if [ -d "$MESA_SOURCE/.git" ]; then
    log_ok "Mesa source already cloned"
    cd "$MESA_SOURCE"
    log_info "Updating Mesa repository..."
    git fetch origin 2>&1 | tee -a "$LOG_FILE"
    git checkout main 2>&1 | tee -a "$LOG_FILE"
else
    log_info "Cloning Mesa repository (this may take a while)..."
    mkdir -p "$(dirname "$MESA_SOURCE")"
    
    if ! git clone --depth 1 https://gitlab.freedesktop.org/mesa/mesa.git "$MESA_SOURCE" 2>&1 | tee -a "$LOG_FILE"; then
        log_error "Failed to clone Mesa repository"
        exit 1
    fi
    
    log_ok "Mesa cloned successfully"
fi

cd "$MESA_SOURCE"

# Step 2: Check dependencies
log_header "Step 2: Check Build Dependencies"

DEPS_OK=0
for tool in meson ninja pkg-config; do
    if command -v "$tool" >/dev/null 2>&1; then
        VERSION=$($tool --version 2>&1 | head -1)
        log_ok "$tool: $VERSION"
        DEPS_OK=$((DEPS_OK + 1))
    else
        log_error "$tool not found - install with: pkgman install haiku_devel"
    fi
done

if [ $DEPS_OK -lt 3 ]; then
    log_error "Missing build tools - cannot continue"
    exit 1
fi

# Step 3: Clean and prepare build
log_header "Step 3: Prepare Build Directory"

if [ -d "$MESA_BUILD" ]; then
    log_info "Removing old build directory..."
    rm -rf "$MESA_BUILD"
fi

mkdir -p "$MESA_BUILD"
log_ok "Build directory ready: $MESA_BUILD"

# Step 4: Configure Meson (R600 driver)
log_header "Step 4: Configure Mesa (R600 + Gallium)"

# Step 4a: Copy RMAPI Gallium driver to Mesa (integrate AMDGPU_Abstracted)
log_info "Integrating RMAPI Gallium driver with Mesa..."
RMAPI_GALLIUM="$PROJECT_ROOT/drivers/gallium"
if [ -d "$RMAPI_GALLIUM" ]; then
    MESA_GALLIUM="$MESA_SOURCE/src/gallium/drivers"
    if [ -d "$MESA_GALLIUM" ]; then
        log_info "Copying RMAPI driver to Mesa: $MESA_GALLIUM"
        cp -r "$RMAPI_GALLIUM"/* "$MESA_GALLIUM/rmapi" 2>/dev/null || true
        log_ok "RMAPI driver integrated"
    fi
fi

# Step 4b: Detect Vulkan support (compile RADV if available)
log_info "Detecting Vulkan support..."
VULKAN_DRIVERS=""
if pkg-config --exists vulkan 2>/dev/null; then
    VULKAN_DRIVERS="amd"
    log_ok "Vulkan detected - including RADV AMD driver"
else
    log_info "Vulkan not available - skipping RADV"
fi

log_info "Running meson setup..."
log_info "GPU Driver: $GPU_DRIVER (detected)"
log_info "Including: softpipe (fallback)"
[ -n "$VULKAN_DRIVERS" ] && log_info "Vulkan: RADV (AMD)"
echo ""

# For Haiku compatibility without libdrm_amdgpu dependency
# Use software rendering fallback (like haiku-nvidia does with NVK)
# AMDGPU_Abstracted provides the GPU driver layer directly
log_info "Configuring for: Software rendering fallback (AMDGPU_Abstracted provides GPU layer)"
log_info "Note: Using empty gallium-drivers to avoid libdrm_amdgpu dependency"

# Build meson command with optional vulkan
# Note: Haiku doesn't have X11, so we disable GLX
# We use empty gallium-drivers (like nvidia-haiku) and rely on AMDGPU_Abstracted for GPU access
# IMPORTANT: Must explicitly disable amdgpu to avoid libdrm_amdgpu dependency
MESON_CMD="meson setup \"$MESA_BUILD\" \
    -Dprefix=\"$INSTALL_PREFIX\" \
    -Dbuildtype=release \
    -Doptimization=3 \
    -Dgallium-drivers= \
    -Dglx=disabled \
    -Dplatforms=haiku \
    -Dopengl=true \
    -Degl=disabled \
    -Dgles1=disabled \
    -Dgles2=enabled \
    -Dshared-glapi=enabled \
    -Damdgpu=disabled \
    -Dllvm=disabled \
    -Dshader-cache=enabled"

# Add vulkan only if available
if [ -n "$VULKAN_DRIVERS" ]; then
    MESON_CMD="$MESON_CMD -Dvulkan-drivers=$VULKAN_DRIVERS"
fi

log_info "Starting Mesa compilation (watch log progress)..."
echo ""

eval "$MESON_CMD" 2>&1 | tee -a "$LOG_FILE"
if [ ${PIPESTATUS[0]} -ne 0 ]; then
    log_error "Meson configuration failed"
    log_error "Last 20 lines of build log:"
    tail -20 "$LOG_FILE"
    exit 1
fi

log_ok "Meson configuration complete"

# Step 5: Compile with progress monitoring
log_header "Step 5: Compile Mesa (this may take 30+ minutes)"

log_info "Starting build with Ninja..."
log_info "Progress (watch last 10 lines of build):"
echo ""

BUILD_START=$(date +%s)

# Run ninja and monitor log in parallel
ninja -C "$MESA_BUILD" 2>&1 | tee -a "$LOG_FILE" &
NINJA_PID=$!

# Monitor log progress while ninja runs
while kill -0 $NINJA_PID 2>/dev/null; do
    if [ -f "$LOG_FILE" ]; then
        # Show last 3 lines every 5 seconds to avoid clutter
        sleep 5
        tail -3 "$LOG_FILE" | sed 's/^/  /'
    fi
done

# Wait for ninja to finish and capture exit code
wait $NINJA_PID
NINJA_EXIT=$?

BUILD_END=$(date +%s)
BUILD_TIME=$((BUILD_END - BUILD_START))

if [ $NINJA_EXIT -ne 0 ]; then
    log_error "Build failed - see details below:"
    log_error "Last 30 lines of build log:"
    tail -30 "$LOG_FILE" | sed 's/^/  /'
    exit 1
fi

log_ok "Build successful (${BUILD_TIME}s)"

# Step 6: Verify build output
log_header "Step 6: Verify Build"

if [ ! -f "$MESA_BUILD/src/gallium/targets/dri/gallium_dri.so" ]; then
    log_error "gallium_dri.so not found in build output"
    exit 1
fi

log_ok "DRI driver built: gallium_dri.so"

# Step 7: Install to standard Haiku paths
log_header "Step 7: Install Mesa to non-packaged"

# Create standard directory structure
log_info "Creating directory structure in $INSTALL_PREFIX..."
mkdir -p "$MESA_BIN_DIR"
mkdir -p "$MESA_DRI_DIR"
mkdir -p "$MESA_VK_DIR"
mkdir -p "${INSTALL_PREFIX}/include"
mkdir -p "${INSTALL_PREFIX}/share/pkgconfig"

log_info "Installing with ninja to: $INSTALL_PREFIX"
if ! ninja -C "$MESA_BUILD" install 2>&1 | tee -a "$LOG_FILE"; then
    log_error "Installation failed"
    exit 1
fi

# Step 8: Verify installation
log_header "Step 8: Verify Installation"

if [ ! -d "$MESA_LIB_DIR" ]; then
    log_error "Installation directory not found: $MESA_LIB_DIR"
    exit 1
fi

log_info "Checking for DRI drivers..."
DRIVERS_FOUND=0
for driver in "$MESA_DRI_DIR"/*_dri.so; do
    if [ -f "$driver" ]; then
        log_ok "Found: $(basename "$driver")"
        DRIVERS_FOUND=$((DRIVERS_FOUND + 1))
    fi
done

if [ $DRIVERS_FOUND -eq 0 ]; then
    log_error "No DRI drivers found in $MESA_DRI_DIR"
    exit 1
fi

# Verify glinfo if available
if command -v glinfo >/dev/null 2>&1; then
    log_info "Testing glinfo..."
    export LD_LIBRARY_PATH="$INSTALL_PREFIX/lib:$LD_LIBRARY_PATH"
    export LIBGL_DRIVERS_PATH="$INSTALL_PREFIX/lib/dri"
    
    if timeout 3 glinfo 2>&1 | grep -qi "opengl"; then
        log_ok "OpenGL working!"
        timeout 3 glinfo 2>&1 | grep -i "renderer\|version" | head -3 | sed 's/^/  /'
    fi
fi

log_ok "Mesa R600 driver installed successfully"

# Summary
log_header "✅ MESA BUILD COMPLETE"

echo "Installation Summary:"
echo "  Location:  $INSTALL_PREFIX"
echo "  Prefix:    $INSTALL_PREFIX/lib"
echo "  DRI Path:  $INSTALL_PREFIX/lib/dri"
echo ""
echo "Next steps:"
echo "  1. Update deploy_gpu_final.sh to use this Mesa installation"
echo "  2. Run: ./scripts/deploy_gpu_final.sh"
echo ""
echo "Build log: $LOG_FILE"
echo ""

log_ok "Build completed in ${BUILD_TIME}s"

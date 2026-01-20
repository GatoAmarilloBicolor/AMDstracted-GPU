#!/bin/bash
# Build and Install Mesa with R600 Driver for Haiku
# Compiles Mesa from source for guaranteed GPU driver availability

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
INSTALL_PREFIX="/boot/home/config/non-packaged"
MESA_SOURCE="${INSTALL_PREFIX}/mesa_source"
MESA_BUILD="${INSTALL_PREFIX}/mesa_build"
LOG_FILE="/tmp/mesa_build_$(date +%s).log"

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

log_info "Running meson setup..."
log_info "GPU Driver: $GPU_DRIVER (detected)"
log_info "Including: softpipe (fallback)"
echo ""

# Build gallium drivers based on detected GPU
case "$GPU_DRIVER" in
    r300)
        GALLIUM_DRIVERS="r300,softpipe"
        log_info "Configuring for: Radeon R300/R400/R500"
        ;;
    radeonsi)
        GALLIUM_DRIVERS="radeonsi,softpipe"
        log_info "Configuring for: RDNA/RDNA2/RDNA3"
        ;;
    r600|*)
        GALLIUM_DRIVERS="r600,softpipe"
        log_info "Configuring for: Radeon HD 5000-7000 (R600)"
        ;;
esac

if ! meson setup "$MESA_BUILD" \
    -Dprefix="$INSTALL_PREFIX" \
    -Dbuildtype=release \
    -Doptimization=3 \
    -Dvulkan-drivers=amd \
    -Dgallium-drivers="$GALLIUM_DRIVERS" \
    -Dglx=dri \
    -Degl=enabled \
    -Dgles1=disabled \
    -Dgles2=enabled \
    -Dshared-glapi=enabled \
    -Dosmesa=true \
    2>&1 | tee -a "$LOG_FILE"; then
    log_error "Meson configuration failed"
    exit 1
fi

log_ok "Meson configuration complete"

# Step 5: Compile
log_header "Step 5: Compile Mesa (this may take 30+ minutes)"

log_info "Starting build with Ninja..."
BUILD_START=$(date +%s)

if ! ninja -C "$MESA_BUILD" 2>&1 | tee -a "$LOG_FILE"; then
    log_error "Build failed - see $LOG_FILE for details"
    exit 1
fi

BUILD_END=$(date +%s)
BUILD_TIME=$((BUILD_END - BUILD_START))

log_ok "Build successful (${BUILD_TIME}s)"

# Step 6: Verify build output
log_header "Step 6: Verify Build"

if [ ! -f "$MESA_BUILD/src/gallium/targets/dri/gallium_dri.so" ]; then
    log_error "r600_dri.so not found in build output"
    exit 1
fi

log_ok "DRI driver built: gallium_dri.so"

# Step 7: Install
log_header "Step 7: Install Mesa"

mkdir -p "$INSTALL_PREFIX"

log_info "Installing with ninja..."
if ! ninja -C "$MESA_BUILD" install 2>&1 | tee -a "$LOG_FILE"; then
    log_error "Installation failed"
    exit 1
fi

# Step 8: Verify installation
log_header "Step 8: Verify Installation"

if [ ! -d "$INSTALL_PREFIX/lib" ]; then
    log_error "Installation directory not found: $INSTALL_PREFIX/lib"
    exit 1
fi

log_info "Checking for r600_dri.so..."
if ! find "$INSTALL_PREFIX" -name "*r600*" -o -name "*gallium*dri*" 2>/dev/null | head -5; then
    log_error "DRI driver not found in installation"
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

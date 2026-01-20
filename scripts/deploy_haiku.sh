#!/bin/bash
# Deploy AMDGPU_Abstracted to Haiku system
# Includes: Core library, RMAPI server, Accelerant module, Mesa, Headers
# Follows deployment patterns from haiku-nvidia

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Helper functions
log_info() { echo -e "${BLUE}[INFO]${NC} $*"; }
log_ok() { echo -e "${GREEN}[✓]${NC} $*"; }
log_error() { echo -e "${RED}[✗]${NC} $*"; }
log_warn() { echo -e "${YELLOW}[⚠]${NC} $*"; }
log_header() { echo -e "\n${BLUE}════════════════════════════════════════════════════════════${NC}\n${BLUE}$*${NC}\n${BLUE}════════════════════════════════════════════════════════════${NC}\n"; }

trap 'log_error "Deployment failed"; exit 1' ERR

# Configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="$(getarch)"
BUILD_DIR="${PROJECT_ROOT}/build.$ARCH"
ACCELERANT_BUILD="${PROJECT_ROOT}/builddir_accelerant"
MESA_BUILD="${PROJECT_ROOT}/mesa_build"
MESA_INSTALL="${PROJECT_ROOT}/install.$ARCH"
INSTALL_PREFIX="${1:-/boot/home/config/non-packaged}"

TARGET_LIB_DIR="${INSTALL_PREFIX}/lib"
TARGET_BIN_DIR="${INSTALL_PREFIX}/bin"
TARGET_INCLUDE_DIR="${INSTALL_PREFIX}/include"
TARGET_ACCELERANT_DIR="${INSTALL_PREFIX}/add-ons/accelerants"

log_header "DEPLOY AMDGPU_ABSTRACTED TO HAIKU"

log_info "Project:     $PROJECT_ROOT"
log_info "Architecture: $ARCH"
log_info "Install to:  $INSTALL_PREFIX"

# ============================================================================
# Step 1: Verify Build Directory
# ============================================================================

log_header "Step 1: Verify Build Directory"

if [ ! -d "$BUILD_DIR" ]; then
    log_error "Build directory not found: $BUILD_DIR"
    log_info "Run Build.sh first to compile"
    exit 1
fi

log_ok "Build directory exists: $BUILD_DIR"

# ============================================================================
# Step 2: Create Install Directories
# ============================================================================

log_header "Step 2: Create Install Directories"

mkdir -p "$TARGET_LIB_DIR"
mkdir -p "$TARGET_BIN_DIR"
mkdir -p "$TARGET_INCLUDE_DIR"
mkdir -p "$TARGET_ACCELERANT_DIR"
mkdir -p "${INSTALL_PREFIX}/share/pkgconfig"

log_ok "Installation directories created"

# ============================================================================
# Step 3: Deploy AMDGPU_Abstracted Core Libraries
# ============================================================================

log_header "Step 3: Deploy AMDGPU_Abstracted Core Libraries"

# Deploy main AMDGPU library
if [ -f "$BUILD_DIR/libamdgpu.so" ]; then
    log_info "Deploying AMDGPU core library..."
    cp "$BUILD_DIR/libamdgpu.so"* "$TARGET_LIB_DIR/" 2>/dev/null || true
    log_ok "libamdgpu.so deployed"
else
    log_warn "libamdgpu.so not found in $BUILD_DIR"
fi

# Deploy DRM shim (bridges libdrm calls to RMAPI)
if [ -f "$BUILD_DIR/libdrm_amdgpu_shim.so" ]; then
    log_info "Deploying DRM AMDGPU shim..."
    cp "$BUILD_DIR/libdrm_amdgpu_shim.so"* "$TARGET_LIB_DIR/" 2>/dev/null || true
    log_ok "libdrm_amdgpu_shim.so deployed"
else
    log_warn "libdrm_amdgpu_shim.so not found"
fi

# Deploy Radeon DRM shim
if [ -f "$BUILD_DIR/libdrm_radeon_shim.so" ]; then
    log_info "Deploying Radeon DRM shim..."
    cp "$BUILD_DIR/libdrm_radeon_shim.so"* "$TARGET_LIB_DIR/" 2>/dev/null || true
    log_ok "libdrm_radeon_shim.so deployed"
else
    log_warn "libdrm_radeon_shim.so not found"
fi

# ============================================================================
# Step 4: Deploy Executables
# ============================================================================

log_header "Step 4: Deploy AMDGPU_Abstracted Executables"

EXECUTABLES=("amd_rmapi_server" "amd_rmapi_client_demo" "amd_test_suite")

for exe in "${EXECUTABLES[@]}"; do
    if [ -f "$BUILD_DIR/$exe" ]; then
        cp "$BUILD_DIR/$exe" "$TARGET_BIN_DIR/"
        chmod +x "$TARGET_BIN_DIR/$exe"
        log_ok "Deployed: $exe"
    else
        log_warn "Not found: $exe"
    fi
done

# ============================================================================
# Step 5: Deploy Haiku Accelerant Module
# ============================================================================

log_header "Step 5: Deploy Haiku Accelerant Module"

if [ -f "$ACCELERANT_BUILD/amd_gfx.accelerant" ]; then
    log_info "Deploying amd_gfx.accelerant..."
    cp "$ACCELERANT_BUILD/amd_gfx.accelerant" "$TARGET_ACCELERANT_DIR/"
    log_ok "amd_gfx.accelerant deployed to add-ons/accelerants"
else
    log_warn "amd_gfx.accelerant not found"
    log_info "This module is required for Haiku Graphics Server integration"
fi

# ============================================================================
# Step 6: Deploy Mesa Libraries (if available)
# ============================================================================

log_header "Step 6: Deploy Mesa Libraries"

if [ -d "$MESA_INSTALL/lib" ]; then
    log_info "Copying Mesa libraries from $MESA_INSTALL..."
    
    cp "$MESA_INSTALL/lib"/libGL*.so* "$TARGET_LIB_DIR/" 2>/dev/null || true
    cp "$MESA_INSTALL/lib"/libGLX*.so* "$TARGET_LIB_DIR/" 2>/dev/null || true
    cp "$MESA_INSTALL/lib"/libEGL*.so* "$TARGET_LIB_DIR/" 2>/dev/null || true
    cp "$MESA_INSTALL/lib"/libGLESv2*.so* "$TARGET_LIB_DIR/" 2>/dev/null || true
    
    # DRI drivers
    if [ -d "$MESA_INSTALL/lib/dri" ]; then
        mkdir -p "$TARGET_LIB_DIR/dri"
        cp "$MESA_INSTALL/lib/dri"/*.so "$TARGET_LIB_DIR/dri/" 2>/dev/null || true
        log_ok "Mesa DRI drivers deployed"
    fi
    
    log_ok "Mesa libraries deployed"
else
    log_warn "Mesa libraries not found in $MESA_INSTALL"
    log_info "Build Mesa using Build.sh, or compile manually"
fi

# ============================================================================
# Step 7: Install Headers
# ============================================================================

log_header "Step 7: Install Development Headers"

HEADER_DIRS=(
    "$PROJECT_ROOT/src"
    "$PROJECT_ROOT/core"
    "$PROJECT_ROOT/drivers"
    "$PROJECT_ROOT/os"
)

for header_dir in "${HEADER_DIRS[@]}"; do
    if [ -d "$header_dir" ]; then
        log_info "Installing headers from $(basename $header_dir)..."
        find "$header_dir" -name "*.h" -exec cp {} "$TARGET_INCLUDE_DIR/" \; 2>/dev/null || true
    fi
done

if [ -d "$MESA_INSTALL/include" ]; then
    log_info "Installing Mesa headers..."
    cp -r "$MESA_INSTALL/include"/* "$TARGET_INCLUDE_DIR/" 2>/dev/null || true
fi

log_ok "Development headers installed"

# ============================================================================
# Step 8: Install pkg-config Files
# ============================================================================

log_header "Step 8: Install pkg-config Files"

if [ -d "$MESA_INSTALL/lib/pkgconfig" ]; then
    log_info "Installing pkg-config files..."
    cp "$MESA_INSTALL/lib/pkgconfig"/*.pc "${INSTALL_PREFIX}/share/pkgconfig/" 2>/dev/null || true
    log_ok "pkg-config files installed"
fi

# ============================================================================
# Step 9: Verify Installation
# ============================================================================

log_header "Step 9: Verify Installation"

VERIFICATION_OK=true

# Check for accelerant
if [ -f "$TARGET_ACCELERANT_DIR/amd_gfx.accelerant" ]; then
    log_ok "Accelerant module: $TARGET_ACCELERANT_DIR/amd_gfx.accelerant"
else
    log_warn "Accelerant module not installed"
    VERIFICATION_OK=false
fi

# Check for RMAPI server
if [ -f "$TARGET_BIN_DIR/amd_rmapi_server" ]; then
    log_ok "RMAPI server: $TARGET_BIN_DIR/amd_rmapi_server"
else
    log_warn "RMAPI server not found"
    VERIFICATION_OK=false
fi

# Check for core library
if [ -f "$TARGET_LIB_DIR/libamdgpu.so" ]; then
    log_ok "Core library: $TARGET_LIB_DIR/libamdgpu.so"
else
    log_warn "Core library not found"
    VERIFICATION_OK=false
fi

# Check for Mesa
if [ -f "$TARGET_LIB_DIR/libGL.so" ] || [ -L "$TARGET_LIB_DIR/libGL.so" ]; then
    log_ok "Mesa OpenGL: $TARGET_LIB_DIR/libGL.so"
else
    log_warn "Mesa OpenGL library not found"
fi

# ============================================================================
# Step 10: Create Configuration Script
# ============================================================================

log_header "Step 10: Create Environment Configuration Script"

CONFIG_SCRIPT="${INSTALL_PREFIX}/setup_amd_gpu.sh"

cat > "$CONFIG_SCRIPT" << 'EOFCONFIG'
#!/bin/bash
# Setup environment for AMDGPU_Abstracted on Haiku
# Source this script to configure paths

INSTALL_PREFIX="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

# Export GPU-related variables
export LD_LIBRARY_PATH="${INSTALL_PREFIX}/lib:${LD_LIBRARY_PATH:-}"
export PKG_CONFIG_PATH="${INSTALL_PREFIX}/share/pkgconfig:${PKG_CONFIG_PATH:-}"

# Mesa-specific
export LIBGL_DRIVERS_PATH="${INSTALL_PREFIX}/lib/dri"
export MESA_DEBUG="${MESA_DEBUG:-}"

# RMAPI server
export RMAPI_SERVER="${INSTALL_PREFIX}/bin/amd_rmapi_server"

echo "════════════════════════════════════════════════════════════"
echo "AMDGPU_Abstracted Environment Configured"
echo "════════════════════════════════════════════════════════════"
echo ""
echo "Paths:"
echo "  Libraries:       ${INSTALL_PREFIX}/lib"
echo "  DRI Drivers:     ${INSTALL_PREFIX}/lib/dri"
echo "  Accelerant:      ${INSTALL_PREFIX}/add-ons/accelerants"
echo "  Headers:         ${INSTALL_PREFIX}/include"
echo ""
echo "Commands:"
echo "  Start GPU server:"
echo "    ${RMAPI_SERVER} &"
echo ""
echo "  Test OpenGL:"
echo "    glinfo | grep -i radeon"
echo ""
echo "  Run test suite:"
echo "    ${INSTALL_PREFIX}/bin/amd_test_suite"
echo ""
EOFCONFIG

chmod +x "$CONFIG_SCRIPT"
log_ok "Configuration script created: $CONFIG_SCRIPT"

# ============================================================================
# Summary
# ============================================================================

log_header "✅ DEPLOYMENT COMPLETE"

echo "Installation Summary:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Location:         $INSTALL_PREFIX"
echo "Libraries:        $TARGET_LIB_DIR"
echo "Binaries:         $TARGET_BIN_DIR"
echo "Accelerant:       $TARGET_ACCELERANT_DIR"
echo "Headers:          $TARGET_INCLUDE_DIR"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Next Steps:"
echo ""
echo "1. Configure environment:"
echo "   source $CONFIG_SCRIPT"
echo ""
echo "2. Start the RMAPI GPU server:"
echo "   $TARGET_BIN_DIR/amd_rmapi_server &"
echo ""
echo "3. Verify GPU is detected:"
echo "   glinfo | grep -i radeon"
echo ""
echo "4. Check accelerant loaded:"
echo "   ls -la $TARGET_ACCELERANT_DIR/"
echo ""
echo "5. Run the test suite:"
echo "   $TARGET_BIN_DIR/amd_test_suite"
echo ""

if [ "$VERIFICATION_OK" = false ]; then
    log_warn "Some components may not be installed correctly"
    log_info "Check the build output above for details"
fi

log_ok "Ready to use AMDGPU_Abstracted on Haiku!"

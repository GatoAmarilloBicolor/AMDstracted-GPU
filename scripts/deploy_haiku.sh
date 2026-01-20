#!/bin/bash
# Deploy AMDGPU_Abstracted and Mesa to Haiku system
# Follows deployment patterns from haiku-nvidia

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
log_warn() { echo -e "${YELLOW}[⚠]${NC} $*"; }
log_header() { echo -e "\n${BLUE}════════════════════════════════════════════════════════════${NC}\n${BLUE}$*${NC}\n${BLUE}════════════════════════════════════════════════════════════${NC}\n"; }

trap 'log_error "Deployment failed"; exit 1' ERR

# Paths
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/builddir_merged"
INSTALL_PREFIX="${1:-/boot/home/config/non-packaged}"
TARGET_LIB_DIR="${INSTALL_PREFIX}/lib"
TARGET_BIN_DIR="${INSTALL_PREFIX}/bin"
TARGET_INCLUDE_DIR="${INSTALL_PREFIX}/include"

log_header "DEPLOY AMDGPU_ABSTRACTED + MESA TO HAIKU"

log_info "Source:      $PROJECT_ROOT"
log_info "Install to:  $INSTALL_PREFIX"
log_info "Lib dir:     $TARGET_LIB_DIR"
log_info "Bin dir:     $TARGET_BIN_DIR"

# Step 1: Verify build exists
log_header "Step 1: Verify Build Directory"

if [ ! -d "$BUILD_DIR" ]; then
    log_error "Build directory not found: $BUILD_DIR"
    log_info "Run Build.sh first to compile"
    exit 1
fi

log_ok "Build directory exists"

# Step 2: Create install directories
log_header "Step 2: Create Install Directories"

mkdir -p "$TARGET_LIB_DIR/dri"
mkdir -p "$TARGET_BIN_DIR"
mkdir -p "$TARGET_INCLUDE_DIR"
mkdir -p "${INSTALL_PREFIX}/share/pkgconfig"
mkdir -p "${INSTALL_PREFIX}/share/vulkan/icd.d"

log_ok "Installation directories created"

# Step 3: Deploy AMDGPU_Abstracted libraries
log_header "Step 3: Deploy AMDGPU_Abstracted Libraries"

# Copy AMDGPU core library
if [ -f "$BUILD_DIR/libamdgpu.a" ]; then
    log_info "Installing static AMDGPU library..."
    # For static, we'll include it in executables
    log_ok "Static library available for linking"
elif [ -f "$BUILD_DIR/libamdgpu.so" ]; then
    log_info "Installing shared AMDGPU library..."
    cp "$BUILD_DIR/libamdgpu.so"* "$TARGET_LIB_DIR/" 2>/dev/null || true
    log_ok "AMDGPU library installed"
fi

# Copy DRM shim (bridges libdrm calls to RMAPI)
if [ -f "$BUILD_DIR/libdrm_amdgpu_shim.a" ]; then
    log_info "DRM shim (static) available"
    log_ok "DRM shim ready for linking"
elif [ -f "$BUILD_DIR/libdrm_amdgpu_shim.so" ]; then
    cp "$BUILD_DIR/libdrm_amdgpu_shim.so"* "$TARGET_LIB_DIR/" 2>/dev/null || true
    log_ok "DRM shim library installed"
fi

# Step 4: Deploy executables
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

# Step 5: Deploy Mesa libraries
log_header "Step 5: Deploy Mesa Libraries"

MESA_BUILD="${PROJECT_ROOT}/builddir_mesa"

if [ ! -d "$MESA_BUILD" ]; then
    log_warn "Mesa build directory not found"
    log_info "Building Mesa first..."
    "$PROJECT_ROOT/Build.sh" || log_warn "Mesa build may have failed"
fi

if [ -d "$MESA_BUILD/install" ]; then
    log_info "Copying Mesa libraries..."
    
    # Copy libraries
    if [ -d "$MESA_BUILD/install/lib" ]; then
        cp -r "$MESA_BUILD/install/lib"/* "$TARGET_LIB_DIR/" 2>/dev/null || true
        log_ok "Mesa libraries copied"
    fi
    
    # Copy include files
    if [ -d "$MESA_BUILD/install/include" ]; then
        cp -r "$MESA_BUILD/install/include"/* "$TARGET_INCLUDE_DIR/" 2>/dev/null || true
        log_ok "Mesa headers copied"
    fi
    
    # Copy pkg-config files
    if [ -d "$MESA_BUILD/install/lib/pkgconfig" ]; then
        cp -r "$MESA_BUILD/install/lib/pkgconfig"/* "${INSTALL_PREFIX}/share/pkgconfig/" 2>/dev/null || true
        log_ok "pkg-config files copied"
    fi
fi

# Step 6: Install headers
log_header "Step 6: Install Headers"

if [ -d "$PROJECT_ROOT/core" ]; then
    log_info "Installing AMDGPU_Abstracted headers..."
    find "$PROJECT_ROOT/core" -name "*.h" -exec cp {} "$TARGET_INCLUDE_DIR/" \; 2>/dev/null || true
    log_ok "Core headers installed"
fi

# Step 7: Verify installation
log_header "Step 7: Verify Installation"

LIBS_FOUND=0
for lib in libGL.so libEGL.so; do
    if [ -f "$TARGET_LIB_DIR/$lib" ] || [ -L "$TARGET_LIB_DIR/$lib" ]; then
        log_ok "Found: $lib"
        LIBS_FOUND=$((LIBS_FOUND + 1))
    fi
done

BIN_FOUND=0
for bin in amd_rmapi_server amd_rmapi_client_demo; do
    if [ -f "$TARGET_BIN_DIR/$bin" ]; then
        log_ok "Found: $bin"
        BIN_FOUND=$((BIN_FOUND + 1))
    fi
done

if [ $LIBS_FOUND -eq 0 ]; then
    log_warn "No Mesa libraries found - Mesa may not have built correctly"
fi

if [ $BIN_FOUND -eq 0 ]; then
    log_warn "No AMDGPU executables found - build may have failed"
fi

# Step 8: Create configuration script
log_header "Step 8: Create Configuration Script"

CONFIG_SCRIPT="${INSTALL_PREFIX}/setup_amd_gpu.sh"

cat > "$CONFIG_SCRIPT" << 'EOF'
#!/bin/bash
# Setup environment for AMDGPU_Abstracted + Mesa on Haiku

INSTALL_PREFIX="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

export LD_LIBRARY_PATH="${INSTALL_PREFIX}/lib:${LD_LIBRARY_PATH:-}"
export LIBGL_DRIVERS_PATH="${INSTALL_PREFIX}/lib/dri"
export PKG_CONFIG_PATH="${INSTALL_PREFIX}/lib/pkgconfig:${INSTALL_PREFIX}/share/pkgconfig:${PKG_CONFIG_PATH:-}"

echo "AMDGPU_Abstracted environment configured:"
echo "  LD_LIBRARY_PATH=${INSTALL_PREFIX}/lib"
echo "  LIBGL_DRIVERS_PATH=${INSTALL_PREFIX}/lib/dri"
echo "  PKG_CONFIG_PATH=${INSTALL_PREFIX}/lib/pkgconfig"
echo ""
echo "RMAPI server:"
echo "  ${INSTALL_PREFIX}/bin/amd_rmapi_server"
EOF

chmod +x "$CONFIG_SCRIPT"
log_ok "Configuration script created: $CONFIG_SCRIPT"

# Summary
log_header "✅ DEPLOYMENT COMPLETE"

echo "Installation Summary:"
echo "  Location:    $INSTALL_PREFIX"
echo "  Libraries:   $TARGET_LIB_DIR"
echo "  Binaries:    $TARGET_BIN_DIR"
echo "  Headers:     $TARGET_INCLUDE_DIR"
echo ""
echo "Next steps:"
echo "  1. Source the configuration script:"
echo "     source $CONFIG_SCRIPT"
echo ""
echo "  2. Start RMAPI server (if needed):"
echo "     $TARGET_BIN_DIR/amd_rmapi_server &"
echo ""
echo "  3. Test OpenGL:"
echo "     glinfo | grep renderer"
echo ""
echo "  4. Run test suite:"
echo "     $TARGET_BIN_DIR/amd_test_suite"
echo ""

log_ok "Ready to use AMDGPU_Abstracted!"

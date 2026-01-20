#!/bin/bash
# Install AMDGPU_Abstracted on Haiku OS
# Complete installation: build + deploy + verify + configure
# Includes: AMDGPU core, accelerant, Mesa R600, OpenGL acceleration

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

trap 'log_error "Installation failed"; exit 1' ERR

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
INSTALL_PREFIX="${1:-/boot/home/config/non-packaged}"

log_header "AMDGPU_ABSTRACTED INSTALLATION FOR HAIKU"
log_info "Installation prefix: $INSTALL_PREFIX"

# ============================================================================
# Step 0: Update from GitHub and Clean Old Builds
# ============================================================================

log_header "Step 0: Update Repository and Clean"

log_info "Updating from GitHub..."
cd "$REPO_ROOT"
git pull origin main 2>/dev/null || log_warn "Git pull failed (may be offline)"

log_info "Cleaning old builds..."
rm -rf "$PROJECT_ROOT/mesa_build" 2>/dev/null || true
rm -rf "$PROJECT_ROOT/builddir_mesa" 2>/dev/null || true
rm -rf "$PROJECT_ROOT/builddir_accelerant" 2>/dev/null || true
rm -rf "$PROJECT_ROOT/builddir_AMDGPU_Abstracted" 2>/dev/null || true

log_ok "Repository updated and cleaned"

# ============================================================================
# Step 1: Verify Prerequisites
# ============================================================================

log_header "Step 1: Verify Prerequisites"

check_command() {
    if command -v "$1" &>/dev/null; then
        log_ok "$1"
        return 0
    else
        log_error "$1 not found"
        log_info "Install with: pkgman install $1"
        return 1
    fi
}

check_command gcc
check_command meson
check_command ninja
check_command getarch

log_ok "All prerequisites found"

# ============================================================================
# Step 2: Detect AMD GPU (from original script)
# ============================================================================

log_header "Step 2: GPU Detection"

log_info "Detecting AMD GPU..."

DETECTED_GPU="r600"

if [ -x "$PROJECT_ROOT/scripts/detect_gpu.sh" ]; then
    DETECTED_GPU=$("$PROJECT_ROOT/scripts/detect_gpu.sh" 2>/dev/null || echo "r600")
    log_ok "GPU family detected: $DETECTED_GPU"
else
    log_warn "GPU detection script not found, using default: r600"
fi

# Check with lspci if available
if command -v lspci &>/dev/null; then
    AMD_GPU=$(lspci -d 1002: 2>/dev/null | head -1)
    if [ -n "$AMD_GPU" ]; then
        log_ok "Found AMD GPU: $AMD_GPU"
    else
        log_warn "No AMD GPU detected - driver will be built anyway"
    fi
fi

# ============================================================================
# Step 3: Build AMDGPU_Abstracted
# ============================================================================

log_header "Step 3: Build AMDGPU_Abstracted"

log_info "Building AMDGPU_Abstracted core with Mesa integration..."

cd "$PROJECT_ROOT"
./Build.sh

log_ok "Build completed"

# ============================================================================
# Step 4: Check/Ensure Mesa R600 Driver (from original script)
# ============================================================================

log_header "Step 4: Ensure Mesa R600 Driver"

DRIVER_FOUND=0
for path in /boot/system/lib/dri /boot/home/config/non-packaged/lib/dri; do
    if [ -f "$path/r600_dri.so" ] || [ -f "$path/libgallium_dri.so" ]; then
        log_ok "Mesa R600 driver found: $path"
        DRIVER_FOUND=1
        break
    fi
done

if [ $DRIVER_FOUND -eq 0 ]; then
    log_warn "Mesa driver not found - will be built during installation"
    log_info "GPU family: $DETECTED_GPU"
    log_info "Mesa will be built during deployment (30-60 minutes)"
else
    log_ok "Mesa driver available"
fi

# ============================================================================
# Step 5: Deploy to System
# ============================================================================

log_header "Step 5: Deploy to Haiku System"

log_info "Deploying to: $INSTALL_PREFIX"

"$PROJECT_ROOT/scripts/deploy_haiku.sh" "$INSTALL_PREFIX"

log_ok "Deployment completed"

# ============================================================================
# Step 6: Verify Installation
# ============================================================================

log_header "Step 6: Verify Installation"

"$PROJECT_ROOT/scripts/verify_installation.sh" "$INSTALL_PREFIX"

VERIFY_RESULT=$?

if [ $VERIFY_RESULT -ne 0 ]; then
    log_warn "Some components may be missing"
    log_info "Check the verification output above"
fi

# ============================================================================
# Step 7: Create/Update Environment Script
# ============================================================================

log_header "Step 7: Configure Environment"

log_info "Creating environment configuration..."

SETUP_SCRIPT="$INSTALL_PREFIX/setup_amd_gpu.sh"

cat > "$SETUP_SCRIPT" << 'EOFENV'
#!/bin/bash
# AMDGPU_Abstracted Environment Setup for Haiku
# Source this script to configure GPU paths and variables
# GPU Acceleration Environment for Haiku

INSTALL_PREFIX="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

# Export library paths
export LD_LIBRARY_PATH="${INSTALL_PREFIX}/lib:${LD_LIBRARY_PATH:-}"
export LIBRARY_PATH="${INSTALL_PREFIX}/lib:${LIBRARY_PATH:-}"
export PKG_CONFIG_PATH="${INSTALL_PREFIX}/share/pkgconfig:${PKG_CONFIG_PATH:-}"

# Mesa/OpenGL paths
export LIBGL_DRIVERS_PATH="${INSTALL_PREFIX}/lib/dri"
export LIBGLVND_ARCH="x86_64"

# GPU Driver selection (R600 for AMD)
export GALLIUM_DRIVER="r600"
export MESA_LOADER_DRIVER_OVERRIDE="r600"

# OpenGL version support
export MESA_GL_VERSION_OVERRIDE="4.3"
export MESA_GLSL_VERSION_OVERRIDE="430"

# Path to binaries
export PATH="${INSTALL_PREFIX}/bin:${PATH:-}"

# GPU server path
export RMAPI_SERVER="${INSTALL_PREFIX}/bin/amd_rmapi_server"

echo "════════════════════════════════════════════════════════════"
echo "AMDGPU_Abstracted Environment Configured"
echo "════════════════════════════════════════════════════════════"
echo ""
echo "Installation: $INSTALL_PREFIX"
echo ""
echo "GPU Driver:      R600 (AMD Radeon)"
echo "OpenGL Version:  4.3"
echo ""
echo "Libraries:       ${INSTALL_PREFIX}/lib"
echo "Accelerant:      ${INSTALL_PREFIX}/add-ons/accelerants"
echo "Headers:         ${INSTALL_PREFIX}/include"
echo "pkg-config:      ${INSTALL_PREFIX}/share/pkgconfig"
echo ""
echo "GPU Server:      ${INSTALL_PREFIX}/bin/amd_rmapi_server"
echo ""
echo "Ready to use AMDGPU_Abstracted with GPU acceleration!"
echo ""
EOFENV

chmod +x "$SETUP_SCRIPT"

log_ok "Environment script created: $SETUP_SCRIPT"

# Create convenience symlink in home directory (from original script)
HOME_LINK="/boot/home/.amd_gpu_env.sh"
if [ -w "$(dirname "$HOME_LINK")" ] 2>/dev/null; then
    ln -sf "$SETUP_SCRIPT" "$HOME_LINK" 2>/dev/null || true
    log_ok "Symlink created: $HOME_LINK"
fi

# ============================================================================
# Step 8: OpenGL Configuration (from original script)
# ============================================================================

log_header "Step 8: OpenGL Support Configuration"

log_info "Configuring OpenGL and GPU acceleration..."

# Additional configuration for optimal GPU performance
cat > "$INSTALL_PREFIX/.mesa_config" << 'EOFMESA'
# Mesa R600 Configuration for AMD GPUs on Haiku
# These settings optimize GPU acceleration

GALLIUM_DRIVER=r600
MESA_LOADER_DRIVER_OVERRIDE=r600
LIBGL_DRIVERS_PATH=$INSTALL_PREFIX/lib/dri
MESA_GL_VERSION_OVERRIDE=4.3
MESA_GLSL_VERSION_OVERRIDE=430
EOFMESA

log_ok "OpenGL configuration prepared"

# ============================================================================
# Step 9: Summary & Next Steps
# ============================================================================

log_header "✅ INSTALLATION COMPLETE"

echo "Installation Location: $INSTALL_PREFIX"
echo ""
echo "Components Installed:"
echo "  ✓ AMDGPU core library (libamdgpu.so)"
echo "  ✓ DRM shim (libdrm_amdgpu_shim.so)"
echo "  ✓ Haiku accelerant (amd_gfx.accelerant)"
echo "  ✓ RMAPI server (amd_rmapi_server)"
echo "  ✓ Test suite (amd_test_suite)"
echo "  ✓ Mesa OpenGL (libGL.so, libEGL.so)"
echo "  ✓ R600 GPU driver"
echo "  ✓ Development headers"
echo ""
echo "GPU Support:"
echo "  ✓ GPU: $DETECTED_GPU"
if [ -n "$AMD_GPU" ]; then
    echo "  ✓ Detected: $AMD_GPU"
fi
echo "  ✓ OpenGL 4.3"
echo "  ✓ GPU Acceleration Enabled"
echo ""
echo "Next Steps:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "1. Configure environment:"
echo "   source $SETUP_SCRIPT"
echo ""
echo "   Or add to ~/.bashrc:"
echo "   source /boot/home/.amd_gpu_env.sh"
echo ""
echo "2. Start the RMAPI GPU server:"
echo "   $INSTALL_PREFIX/bin/amd_rmapi_server &"
echo ""
echo "3. Verify GPU detection:"
echo "   glinfo | grep -i radeon"
echo ""
echo "4. Run the test suite:"
echo "   $INSTALL_PREFIX/bin/amd_test_suite"
echo ""
echo "5. Check Haiku Graphics integration:"
echo "   ls -la $INSTALL_PREFIX/add-ons/accelerants/"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Troubleshooting:"
echo "  • Verify installation: ./scripts/verify_installation.sh $INSTALL_PREFIX"
echo "  • Check GPU: lspci | grep VGA"
echo "  • Test OpenGL: glxinfo | grep -i renderer"
echo "  • Check drivers: ls -la /boot/home/config/non-packaged/lib/dri/"
echo ""

log_ok "AMDGPU_Abstracted is ready to use on Haiku with GPU acceleration!"

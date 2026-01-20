#!/bin/bash
# Install AMDGPU_Abstracted on Haiku OS
# Complete installation: build + deploy + verify + configure
# Recommended: Use this script for one-command installation on Haiku

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
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INSTALL_PREFIX="${1:-/boot/home/config/non-packaged}"

log_header "AMDGPU_ABSTRACTED INSTALLATION FOR HAIKU"

log_info "Installation prefix: $INSTALL_PREFIX"

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
# Step 2: Build AMDGPU_Abstracted
# ============================================================================

log_header "Step 2: Build AMDGPU_Abstracted"

log_info "Building AMDGPU_Abstracted core..."

cd "$PROJECT_ROOT"
./Build.sh

log_ok "Build completed"

# ============================================================================
# Step 3: Deploy to System
# ============================================================================

log_header "Step 3: Deploy to Haiku System"

log_info "Deploying to: $INSTALL_PREFIX"

./scripts/deploy_haiku.sh "$INSTALL_PREFIX"

log_ok "Deployment completed"

# ============================================================================
# Step 4: Verify Installation
# ============================================================================

log_header "Step 4: Verify Installation"

./scripts/verify_installation.sh "$INSTALL_PREFIX"

VERIFY_RESULT=$?

if [ $VERIFY_RESULT -ne 0 ]; then
    log_warn "Some components may be missing"
    log_info "Check the verification output above"
fi

# ============================================================================
# Step 5: Create/Update Environment Script
# ============================================================================

log_header "Step 5: Configure Environment"

log_info "Creating environment configuration..."

SETUP_SCRIPT="$INSTALL_PREFIX/setup_amd_gpu.sh"

cat > "$SETUP_SCRIPT" << 'EOFENV'
#!/bin/bash
# AMDGPU_Abstracted Environment Setup for Haiku
# Source this script to configure GPU paths and variables

INSTALL_PREFIX="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

# Export library paths
export LD_LIBRARY_PATH="${INSTALL_PREFIX}/lib:${LD_LIBRARY_PATH:-}"
export LIBRARY_PATH="${INSTALL_PREFIX}/lib:${LIBRARY_PATH:-}"
export PKG_CONFIG_PATH="${INSTALL_PREFIX}/share/pkgconfig:${PKG_CONFIG_PATH:-}"

# Mesa/OpenGL paths
export LIBGL_DRIVERS_PATH="${INSTALL_PREFIX}/lib/dri"
export LIBGLVND_ARCH="x86_64"

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
echo "Libraries:       ${INSTALL_PREFIX}/lib"
echo "Accelerant:      ${INSTALL_PREFIX}/add-ons/accelerants"
echo "Headers:         ${INSTALL_PREFIX}/include"
echo "pkg-config:      ${INSTALL_PREFIX}/share/pkgconfig"
echo ""
echo "GPU Server:      ${INSTALL_PREFIX}/bin/amd_rmapi_server"
echo ""
echo "Ready to use AMDGPU_Abstracted!"
echo ""
EOFENV

chmod +x "$SETUP_SCRIPT"

log_ok "Environment script created: $SETUP_SCRIPT"

# Create convenience symlink in home directory
HOME_LINK="/boot/home/.amd_gpu_env.sh"
if [ -w "$(dirname "$HOME_LINK")" ]; then
    ln -sf "$SETUP_SCRIPT" "$HOME_LINK" 2>/dev/null || true
    log_ok "Symlink created: $HOME_LINK"
fi

# ============================================================================
# Step 6: GPU Detection
# ============================================================================

log_header "Step 6: GPU Detection"

if command -v lspci &>/dev/null; then
    log_info "Scanning for AMD GPUs..."
    
    AMD_GPU_COUNT=$(lspci -d 1002: 2>/dev/null | wc -l)
    
    if [ "$AMD_GPU_COUNT" -gt 0 ]; then
        log_ok "Found $AMD_GPU_COUNT AMD GPU(s):"
        lspci -d 1002: 2>/dev/null | while read -r line; do
            echo "  • $line"
        done
    else
        log_warn "No AMD GPUs detected"
        log_info "Check GPU with: lspci -d 1002:"
    fi
else
    log_warn "lspci not found - cannot detect GPU"
    log_info "Install pciutils: pkgman install pciutils"
fi

# ============================================================================
# Step 7: Summary & Next Steps
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
echo "  ✓ Development headers"
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
echo ""

log_ok "AMDGPU_Abstracted is ready to use on Haiku!"

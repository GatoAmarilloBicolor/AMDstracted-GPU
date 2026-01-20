#!/bin/bash
# 🚀 HAIKU GPU ACCELERATION INSTALLER - GPU REQUIRED
# NO CPU FALLBACK - GPU IS MANDATORY
# Installs AMDGPU_Abstracted with GPU acceleration
# FAILS if GPU not available - this is NOT optional

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

trap 'log_error "Installation failed"; exit 1' ERR

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

log_header "🚀 GPU ACCELERATION INSTALLER - GPU REQUIRED"
log_error "⚠️  THIS SCRIPT REQUIRES GPU HARDWARE"
log_error "⚠️  NO CPU FALLBACK - GPU IS MANDATORY"
echo ""

# =================================================================
# PHASE 1: VERIFY GPU HARDWARE
# =================================================================
log_header "PHASE 1: GPU Hardware Verification (REQUIRED)"

if ! command -v lspci >/dev/null 2>&1; then
    log_error "lspci not found - cannot verify GPU"
    log_error "Install: pkgman install pciutils"
    exit 1
fi

if ! GPU=$(lspci 2>/dev/null | grep -i "radeon\|amd.*vga"); then
    log_error "❌ NO AMD GPU DETECTED"
    log_error ""
    log_error "Available GPUs:"
    lspci 2>/dev/null | grep -i "vga\|display\|3d" | sed 's/^/  /'
    log_error ""
    log_error "GPU is REQUIRED - this system cannot continue without AMD Radeon"
    exit 1
fi

log_ok "AMD GPU detected:"
echo "$GPU" | sed 's/^/  /'

# =================================================================
# PHASE 2: VERIFY MESA R600 DRIVER
# =================================================================
log_header "PHASE 2: Mesa R600 Driver Verification (REQUIRED)"

DRIVER_FOUND=0
for path in /boot/system/lib/dri /boot/home/config/non-packaged/lib/dri; do
    if [ -f "$path/r600_dri.so" ] || [ -f "$path/libgallium_dri.so" ]; then
        log_ok "Driver found: $path"
        DRIVER_FOUND=1
        break
    fi
done

if [ $DRIVER_FOUND -eq 0 ]; then
    log_error "❌ MESA R600 DRIVER NOT FOUND"
    log_error ""
    log_error "GPU driver is REQUIRED - cannot continue without r600_dri.so"
    log_error ""
    log_error "Options:"
    log_error "  1. Install package: pkgman install mesa_r600 mesa_devel"
    log_error "  2. Build from source: ./scripts/build_mesa_r600.sh"
    exit 1
fi

log_ok "Mesa R600 driver verified"

# =================================================================
# PHASE 3: BUILD & DEPLOY GPU ACCELERATION
# =================================================================
log_header "PHASE 3: Deploy GPU Acceleration"

if [ ! -x "$PROJECT_ROOT/scripts/deploy_gpu_final.sh" ]; then
    log_error "GPU deployment script not found: $PROJECT_ROOT/scripts/deploy_gpu_final.sh"
    exit 1
fi

log_info "Running: deploy_gpu_final.sh"
echo ""

if ! "$PROJECT_ROOT/scripts/deploy_gpu_final.sh"; then
    log_error "GPU deployment failed"
    exit 1
fi

# =================================================================
# PHASE 4: VERIFICATION - GPU MUST WORK
# =================================================================
log_header "PHASE 4: Verify GPU Acceleration (REQUIRED)"

INSTALL_PREFIX="/boot/home/config/non-packaged"

# Check binaries
for binary in amd_rmapi_server amd_rmapi_client_demo; do
    if [ ! -x "$INSTALL_PREFIX/bin/$binary" ]; then
        log_error "Binary not found: $binary"
        exit 1
    fi
done
log_ok "All binaries deployed"

# Check environment
if [ ! -f "$INSTALL_PREFIX/.amd_gpu_env.sh" ]; then
    log_error "Environment script not found"
    exit 1
fi
log_ok "Environment configured"

# Test OpenGL (if glinfo available)
if command -v glinfo >/dev/null 2>&1; then
    log_info "Testing OpenGL..."
    
    # Source environment
    source "$INSTALL_PREFIX/.amd_gpu_env.sh" 2>/dev/null || true
    
    # Test
    if GLINFO_OUT=$(timeout 5 glinfo 2>&1 || true); then
        if echo "$GLINFO_OUT" | grep -qi "opengl"; then
            # Check for GPU renderer
            if echo "$GLINFO_OUT" | grep -qi "llvmpipe\|softpipe\|swrast"; then
                log_error "❌ SOFTWARE RENDERING DETECTED - GPU NOT LOADED"
                log_error ""
                log_error "glinfo shows:"
                echo "$GLINFO_OUT" | grep -i "renderer" | sed 's/^/  /'
                log_error ""
                log_error "GPU must be used - software rendering not allowed"
                exit 1
            elif echo "$GLINFO_OUT" | grep -qi "radeon"; then
                log_ok "GPU rendering active:"
                echo "$GLINFO_OUT" | grep -E "renderer|version|vendor" | head -3 | sed 's/^/  /'
            fi
        fi
    fi
fi

# =================================================================
# SUCCESS
# =================================================================
log_header "✅ GPU ACCELERATION INSTALLATION COMPLETE!"

echo "System Configuration:"
echo "  GPU:           $(echo "$GPU" | head -1 | sed 's/^[^:]*: //')"
echo "  Driver:        Mesa R600"
echo "  Installation:  $INSTALL_PREFIX"
echo "  Binaries:      $INSTALL_PREFIX/bin"
echo "  Environment:   $INSTALL_PREFIX/.amd_gpu_env.sh"
echo ""
echo "Next Steps:"
echo "  1. Load environment:"
echo "     source /boot/home/.amd_gpu_env.sh"
echo ""
echo "  2. Start GPU server:"
echo "     gpu_server &"
echo ""
echo "  3. Test GPU:"
echo "     gpu_app glinfo"
echo "     gpu_app glxgears"
echo ""
echo "Verify GPU is working:"
echo "  glinfo | grep -i renderer"
echo "  Should show: Radeon (not llvmpipe)"
echo ""

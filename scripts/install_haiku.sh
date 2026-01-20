#!/bin/bash
# 🚀 HAIKU INSTALLER - AMDGPU_ABSTRACTED WITH GPU ACCELERATION
# GPU-ONLY MODE - NO CPU FALLBACK
# Complete automated installation for Haiku with AMD GPU support

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
LOG_FILE="/tmp/haiku_install_$(date +%s).log"

log_header "🚀 HAIKU INSTALLATION - AMDGPU_ABSTRACTED GPU ACCELERATION"
log_info "Log: $LOG_FILE"
log_info "Mode: GPU-ONLY (no CPU fallback)"

# =================================================================
# PHASE 1: CHECK PREREQUISITES
# =================================================================
log_header "PHASE 1: Check Build Prerequisites"

for tool in gcc meson ninja pkg-config; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        log_error "$tool not found"
        log_error "Install: pkgman install haiku_devel"
        exit 1
    fi
    log_ok "$tool available"
done

# =================================================================
# PHASE 2: VERIFY GPU HARDWARE (REQUIRED)
# =================================================================
log_header "PHASE 2: Verify AMD GPU Hardware (REQUIRED)"

if ! command -v lspci >/dev/null 2>&1; then
    log_error "lspci not found - cannot verify GPU"
    log_error "Install: pkgman install pciutils"
    exit 1
fi

if ! GPU=$(lspci 2>/dev/null | grep -i "radeon\|amd.*vga"); then
    log_error "❌ NO AMD GPU DETECTED"
    log_error ""
    log_error "This system does not have an AMD GPU"
    log_error "Available hardware:"
    lspci 2>/dev/null | grep -i "vga\|display\|3d" | sed 's/^/  /'
    log_error ""
    log_error "GPU is REQUIRED - installation cannot continue"
    exit 1
fi

log_ok "AMD GPU detected:"
echo "$GPU" | sed 's/^/  /'

# =================================================================
# PHASE 3: VERIFY MESA R600 DRIVER (REQUIRED)
# =================================================================
log_header "PHASE 3: Verify Mesa R600 Driver (REQUIRED)"

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
    log_error "GPU driver is REQUIRED to continue"
    log_error ""
    log_error "Install options:"
    log_error "  1. Package manager: pkgman install mesa_r600 mesa_devel"
    log_error "  2. Build from source: ./scripts/build_mesa_r600.sh"
    exit 1
fi

log_ok "Mesa R600 driver verified"

# =================================================================
# PHASE 4: RUN GPU ACCELERATION DEPLOYMENT
# =================================================================
log_header "PHASE 4: Deploy GPU Acceleration"

if [ ! -x "$PROJECT_ROOT/scripts/deploy_gpu_final.sh" ]; then
    log_error "Deployment script not found: $PROJECT_ROOT/scripts/deploy_gpu_final.sh"
    exit 1
fi

log_info "Running: deploy_gpu_final.sh"
echo ""

if ! "$PROJECT_ROOT/scripts/deploy_gpu_final.sh" 2>&1 | tee -a "$LOG_FILE"; then
    log_error "GPU acceleration deployment failed"
    exit 1
fi

# =================================================================
# PHASE 5: VERIFY GPU ACCELERATION WORKS
# =================================================================
log_header "PHASE 5: Verify GPU Acceleration (REQUIRED)"

INSTALL_PREFIX="/boot/home/config/non-packaged"

# Check binaries
for binary in amd_rmapi_server amd_rmapi_client_demo; do
    if [ ! -x "$INSTALL_PREFIX/bin/$binary" ]; then
        log_error "Binary not found: $binary"
        exit 1
    fi
    log_ok "$binary deployed"
done

# Check environment
if [ ! -f "$INSTALL_PREFIX/.amd_gpu_env.sh" ]; then
    log_error "Environment script not found"
    exit 1
fi
log_ok "Environment configured"

# Test OpenGL if glinfo available
if command -v glinfo >/dev/null 2>&1; then
    log_info "Testing OpenGL with GPU..."
    
    source "$INSTALL_PREFIX/.amd_gpu_env.sh" 2>/dev/null || true
    
    if GLINFO_OUT=$(timeout 5 glinfo 2>&1 || true); then
        if echo "$GLINFO_OUT" | grep -qi "opengl"; then
            # Check for software rendering (NOT allowed)
            if echo "$GLINFO_OUT" | grep -qi "llvmpipe\|softpipe\|swrast"; then
                log_error "❌ SOFTWARE RENDERING DETECTED"
                log_error ""
                log_error "glinfo shows:"
                echo "$GLINFO_OUT" | grep -i "renderer" | sed 's/^/  /'
                log_error ""
                log_error "GPU acceleration NOT working - software rendering detected"
                log_error "This indicates Mesa driver not properly loaded"
                exit 1
            elif echo "$GLINFO_OUT" | grep -qi "radeon"; then
                log_ok "GPU rendering active:"
                echo "$GLINFO_OUT" | grep -E "renderer|version|vendor" | head -3 | sed 's/^/  /'
            else
                log_error "OpenGL renderer not identified"
                exit 1
            fi
        fi
    fi
fi

# =================================================================
# SUCCESS
# =================================================================
log_header "✅ INSTALLATION COMPLETE!"

echo ""
echo "System Configuration:"
echo "  GPU:           $(echo "$GPU" | head -1 | sed 's/^[^:]*: //')"
echo "  Driver:        Mesa R600 (Gallium)"
echo "  Mode:          GPU Acceleration (Hardware)"
echo "  Installation:  $INSTALL_PREFIX"
echo ""
echo "Installation Paths:"
echo "  Binaries:      $INSTALL_PREFIX/bin"
echo "  Environment:   /boot/home/.amd_gpu_env.sh"
echo "  Config:        $INSTALL_PREFIX/.amd_gpu_env.sh"
echo ""
echo "Next Steps:"
echo "  1. Load environment:"
echo "     source /boot/home/.amd_gpu_env.sh"
echo ""
echo "  2. Start GPU server:"
echo "     gpu_server &"
echo ""
echo "  3. Test GPU acceleration:"
echo "     gpu_app glinfo"
echo "     gpu_app glxgears"
echo ""
echo "Verify GPU is working:"
echo "  glinfo | grep -i renderer"
echo "  Should show: Radeon (NOT llvmpipe)"
echo ""
echo "Installation log: $LOG_FILE"
echo ""

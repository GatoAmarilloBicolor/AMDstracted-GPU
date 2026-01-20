#!/bin/bash
# Universal installation script for AMDGPU_Abstracted
# Detects OS (Linux/Haiku) and runs appropriate installation
# Supports system-wide or user-local installation

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

# Detect OS
OS_NAME="$(uname -s)"

case "$OS_NAME" in
    Haiku)
        log_info "Detected: Haiku OS"
        INSTALL_PREFIX="${1:-/boot/home/config/non-packaged}"
        
        # Run Haiku deployment script
        SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
        "$SCRIPT_DIR/deploy_haiku.sh" "$INSTALL_PREFIX"
        ;;
    
    Linux)
        log_info "Detected: Linux"
        log_warn "Linux deployment requires system dependencies"
        
        # Determine installation prefix
        if [ $EUID -eq 0 ]; then
            DEFAULT_PREFIX="/usr/local"
        else
            DEFAULT_PREFIX="$HOME/.local"
        fi
        
        INSTALL_PREFIX="${1:-$DEFAULT_PREFIX}"
        
        log_info "Installation prefix: $INSTALL_PREFIX"
        
        # Check if build exists
        BUILD_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/build.$(uname -m)"
        
        if [ ! -d "$BUILD_DIR" ]; then
            log_error "Build directory not found: $BUILD_DIR"
            log_info "Run ./Build.sh first to compile"
            exit 1
        fi
        
        # Create directories
        mkdir -p "$INSTALL_PREFIX/lib"
        mkdir -p "$INSTALL_PREFIX/bin"
        mkdir -p "$INSTALL_PREFIX/include"
        
        log_info "Installing AMDGPU_Abstracted core libraries..."
        
        # Install libraries
        if [ -f "$BUILD_DIR/libamdgpu.so" ]; then
            cp "$BUILD_DIR/libamdgpu.so"* "$INSTALL_PREFIX/lib/" 2>/dev/null || true
            log_ok "libamdgpu.so installed"
        fi
        
        # Install executables
        for exe in amd_rmapi_server amd_rmapi_client_demo amd_test_suite; do
            if [ -f "$BUILD_DIR/$exe" ]; then
                cp "$BUILD_DIR/$exe" "$INSTALL_PREFIX/bin/"
                chmod +x "$INSTALL_PREFIX/bin/$exe"
                log_ok "$exe installed"
            fi
        done
        
        log_ok "Installation complete to $INSTALL_PREFIX"
        
        # Configuration
        if [ $EUID -ne 0 ]; then
            log_info "Add to ~/.bashrc:"
            echo "  export LD_LIBRARY_PATH=\"$INSTALL_PREFIX/lib:\$LD_LIBRARY_PATH\""
            echo "  export PATH=\"$INSTALL_PREFIX/bin:\$PATH\""
        else
            log_info "System-wide installation complete"
            echo "  Update /etc/ld.so.conf to include $INSTALL_PREFIX/lib"
            echo "  Run: sudo ldconfig"
        fi
        ;;
    
    FreeBSD)
        log_info "Detected: FreeBSD"
        log_error "FreeBSD support requires additional configuration"
        log_info "See README.md for FreeBSD installation instructions"
        exit 1
        ;;
    
    *)
        log_error "Unsupported operating system: $OS_NAME"
        log_info "Supported: Linux, Haiku, FreeBSD"
        exit 1
        ;;
esac

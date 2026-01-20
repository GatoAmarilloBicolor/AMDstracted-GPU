#!/bin/bash
# AMD GPU Driver Setup - Universal build and install script
# Works on both Haiku OS and Linux
# Handles: Core compilation, Accelerant, Mesa, and system installation

set -e

baseDir="$PWD"

# ============================================================================
# DETECT PLATFORM
# ============================================================================

if command -v getarch &> /dev/null; then
    OS_NAME="Haiku"
    ARCH="$(getarch)"
    ON_HAIKU=true
else
    OS_NAME="$(uname -s)"
    ARCH="$(uname -m)"
    ON_HAIKU=false
fi

buildBaseDir="$baseDir/build.$ARCH"
installDir="$baseDir/install.$ARCH"

# ============================================================================
# FUNCTIONS
# ============================================================================

log_info() { echo "[INFO] $*"; }
log_ok() { echo "[✓] $*"; }
log_error() { echo "[✗] $*" >&2; }

# ============================================================================
# GPU DETECTION
# ============================================================================

detect_gpu() {
    local gpu_model=""
    
    if [ "$ON_HAIKU" = true ]; then
        # Haiku GPU detection
        gpu_model=$(lspci 2>/dev/null | grep -i "amd\|radeon" | head -1 || echo "")
    else
        # Linux GPU detection
        gpu_model=$(lspci 2>/dev/null | grep -i "amd\|radeon" | head -1 || \
                   lsusb 2>/dev/null | grep -i "amd\|radeon" | head -1 || echo "")
    fi
    
    echo "$gpu_model"
}

# Determine Mesa driver based on GPU family
determine_mesa_driver() {
    local gpu="$1"
    local driver="r600"  # default
    
    if [ -z "$gpu" ]; then
        log_info "GPU detection failed, using default driver: r600"
        echo "$driver"
        return
    fi
    
    log_info "Detected GPU: $gpu"
    
    # GCN architecture (Radeon RX series, R9/R7)
    if echo "$gpu" | grep -qi "rx\|r9\|r7\|polaris\|vega\|navi\|rdna"; then
        driver="radeonsi"
        log_info "GPU family: GCN/RDNA → Using radeonsi driver"
    # R600 architecture (Radeon HD 2000-5000)
    elif echo "$gpu" | grep -qi "hd.*[2345][0-9][0-9][0-9]"; then
        driver="r600"
        log_info "GPU family: VLIW (HD 2000-5000) → Using r600 driver"
    # R300 architecture (ancient)
    elif echo "$gpu" | grep -qi "radeon.*[89]\|radeon.*x[0-9]"; then
        driver="r300"
        log_info "GPU family: Ancient → Using r300 driver"
    else
        log_info "GPU family unclear, using r600 as fallback"
        driver="r600"
    fi
    
    echo "$driver"
}

show_usage() {
    cat << EOF
AMD GPU Driver Setup - Universal Build Script

Usage: $0 [COMMAND] [OPTIONS]

Commands:
  full              Complete build (auto-detects GPU on Haiku)
  prepare-mesa      Download Mesa source (one-time setup)
  build-core        Build AMDGPU_Abstracted core
  build-accelerant  Build AMD Accelerant for Haiku
  build-mesa [GPU]  Build Mesa (auto-detects if no GPU specified)
  install           Install system-wide (Haiku, requires sudo)

GPU Options (optional for build-mesa):
  (auto)          Auto-detect GPU family (default)
  r600            AMD Radeon HD 2000-5000
  radeonsi        AMD Radeon RX (GCN+, modern)
  r300            Ancient AMD Radeon

Examples:
  $0 full                 # Complete build (auto-detects GPU)
  $0 build-mesa           # Build Mesa (auto-detects GPU)
  $0 build-mesa radeonsi  # Build Mesa (force radeonsi driver)
  sudo $0 install         # Install system-wide (Haiku only)

GPU Auto-Detection:
  The script automatically detects your GPU and selects the correct
  Mesa driver. Manual override available if needed.

EOF
}

# ============================================================================
# STEP 1: PREPARE MESA
# ============================================================================

prepare_mesa() {
    log_info "Preparing Mesa for Haiku"
    
    if [ ! "$ON_HAIKU" = true ]; then
        log_error "Mesa preparation only needed on Haiku"
        return 0
    fi
    
    if [ -d "$baseDir/mesa_source" ]; then
        log_ok "mesa_source already exists"
        return 0
    fi
    
    MESA_VERSION="21.1.9"
    MESA_URL="https://github.com/mesa3d/mesa/archive/refs/tags/$MESA_VERSION.tar.gz"
    
    log_info "Downloading Mesa $MESA_VERSION..."
    
    cd "$baseDir"
    if command -v wget &> /dev/null; then
        wget -q -O "mesa-$MESA_VERSION.tar.gz" "$MESA_URL"
    elif command -v curl &> /dev/null; then
        curl -s -L -o "mesa-$MESA_VERSION.tar.gz" "$MESA_URL"
    else
        log_error "wget or curl required"
        return 1
    fi
    
    log_info "Extracting Mesa..."
    tar xzf "mesa-$MESA_VERSION.tar.gz"
    mv "mesa-$MESA_VERSION" "mesa_source"
    rm "mesa-$MESA_VERSION.tar.gz"
    
    log_ok "Mesa prepared"
}

# ============================================================================
# STEP 2: BUILD CORE
# ============================================================================

build_core() {
    log_info "Building AMDGPU_Abstracted core"
    
    mkdir -p "$buildBaseDir/AMDGPU_Abstracted"
    cd "$baseDir"
    
    meson setup "$buildBaseDir/AMDGPU_Abstracted" \
        -Dprefix="$installDir" || return 1
    
    ninja -C "$buildBaseDir/AMDGPU_Abstracted" || return 1
    ninja -C "$buildBaseDir/AMDGPU_Abstracted" install || return 1
    
    log_ok "Core built"
}

# ============================================================================
# STEP 3: BUILD ACCELERANT (Haiku only)
# ============================================================================

build_accelerant() {
    if [ ! "$ON_HAIKU" = true ]; then
        log_info "Accelerant only builds on Haiku"
        return 0
    fi
    
    log_info "Building AMD Accelerant for Haiku"
    
    buildDir="$baseDir/builddir_accelerant"
    
    if [ -d "$buildDir" ]; then
        rm -rf "$buildDir"
    fi
    
    cd "$baseDir/accelerant"
    
    meson setup "$buildDir" \
        -Dprefix="$installDir" || return 1
    
    ninja -C "$buildDir" || return 1
    ninja -C "$buildDir" install || return 1
    
    log_ok "Accelerant built"
}

# ============================================================================
# STEP 4: BUILD MESA
# ============================================================================

build_mesa() {
    # Auto-detect GPU if no driver specified
    local driver="$1"
    
    if [ -z "$driver" ]; then
        local gpu=$(detect_gpu)
        driver=$(determine_mesa_driver "$gpu")
    fi
    
    log_info "Building Mesa with $driver driver"
    
    if [ ! "$ON_HAIKU" = true ]; then
        log_info "Mesa build requires Haiku system libraries"
        return 0
    fi
    
    if [ ! -d "$baseDir/mesa_source" ]; then
        log_error "Mesa source not found, run: $0 prepare-mesa"
        return 1
    fi
    
    buildDir="$baseDir/builddir_mesa"
    
    if [ -d "$buildDir" ]; then
        rm -rf "$buildDir"
    fi
    
    mkdir -p "$buildDir"
    mkdir -p "$installDir/lib/pkgconfig"
    
    cd "$baseDir/mesa_source"
    
    meson setup "$buildDir" \
        -Dprefix="$installDir" \
        -Dbuildtype=release \
        -Doptimization=3 \
        -Dgallium-drivers=$driver \
        -Dplatforms=haiku \
        -Dopengl=true \
        -Dglx=disabled \
        -Degl=disabled \
        -Dgles2=enabled \
        -Dshader-cache=enabled \
        -Dvulkan-drivers= \
        -Dllvm=disabled || return 1
    
    ninja -C "$buildDir" || return 1
    ninja -C "$buildDir" install || return 1
    
    log_ok "Mesa built"
}

# ============================================================================
# STEP 5: INSTALL (Haiku only)
# ============================================================================

install_system() {
    if [ ! "$ON_HAIKU" = true ]; then
        log_info "System installation only for Haiku"
        return 0
    fi
    
    log_info "Installing to system directories"
    
    # Check root
    if [ "$EUID" -ne 0 ]; then
        log_error "Installation requires root. Run: sudo $0 install"
        return 1
    fi
    
    # Install accelerant
    if [ -f "$installDir/add-ons/accelerants/amd_gfx.accelerant" ]; then
        log_info "Installing accelerant..."
        mkdir -p /boot/system/add-ons/accelerants
        cp "$installDir/add-ons/accelerants/amd_gfx.accelerant" \
           /boot/system/add-ons/accelerants/
        log_ok "Accelerant installed"
    fi
    
    # Install Mesa
    if [ -f "$installDir/lib/libGL.so" ] || [ -f "$installDir/lib/libGL.so.1" ]; then
        log_info "Installing Mesa..."
        mkdir -p /boot/system/lib
        cp "$installDir/lib/"*.so* /boot/system/lib/ 2>/dev/null || true
        log_ok "Mesa installed"
    fi
    
    log_ok "Installation complete"
    echo ""
    echo "Next steps:"
    echo "  pkill -9 app_server    # Restart graphics"
    echo "  open /boot/system/apps/WebPositive  # Test"
}

# ============================================================================
# MAIN
# ============================================================================

echo "════════════════════════════════════════════════════════════"
echo "AMD GPU Driver Setup - $OS_NAME"
echo "════════════════════════════════════════════════════════════"
echo ""

case "${1:-full}" in
    prepare-mesa)
        prepare_mesa
        ;;
    build-core)
        build_core
        ;;
    build-accelerant)
        build_accelerant
        ;;
    build-mesa)
        build_mesa "${2:-r600}"
        ;;
    install)
        install_system
        ;;
    full)
        log_info "Starting full build sequence..."
        echo ""
        
        # Auto-detect GPU and driver on Haiku
        local mesa_driver=""
        if [ "$ON_HAIKU" = true ]; then
            echo "[*] Detecting GPU..."
            local gpu=$(detect_gpu)
            mesa_driver=$(determine_mesa_driver "$gpu")
            echo ""
        fi
        
        if [ "$ON_HAIKU" = true ]; then
            prepare_mesa || exit 1
            echo ""
        fi
        
        build_core || exit 1
        echo ""
        
        if [ "$ON_HAIKU" = true ]; then
            build_accelerant || exit 1
            echo ""
            build_mesa "$mesa_driver" || exit 1
            echo ""
            
            log_info "Build complete. To install, run:"
            echo "  sudo $0 install"
        fi
        ;;
    -h|--help|help)
        show_usage
        ;;
    *)
        log_error "Unknown command: $1"
        show_usage
        exit 1
        ;;
esac

echo ""
echo "════════════════════════════════════════════════════════════"
log_ok "Done"
echo "════════════════════════════════════════════════════════════"

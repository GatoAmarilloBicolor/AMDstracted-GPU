#!/bin/bash
# FIX for Haiku users: Repairs Build.sh if Mesa build fails
# Run this if you get: "meson: error: unrecognized arguments: mesa_source"

set -e

echo "════════════════════════════════════════════════════════════"
echo "Fix for Haiku Build.sh - Mesa meson configuration"
echo "════════════════════════════════════════════════════════════"
echo ""

PROJECT_ROOT="$PWD"

# Check if Build.sh exists
if [ ! -f "$PROJECT_ROOT/AMDGPU_Abstracted/Build.sh" ]; then
    echo "Error: Not in project root. Run from ~/src/AMDstracted-GPU"
    exit 1
fi

echo "[INFO] Backing up current Build.sh..."
cp "$PROJECT_ROOT/AMDGPU_Abstracted/Build.sh" "$PROJECT_ROOT/AMDGPU_Abstracted/Build.sh.backup"
echo "[✓] Backup saved: Build.sh.backup"

echo "[INFO] Creating fixed Build.sh..."

cat > "$PROJECT_ROOT/AMDGPU_Abstracted/Build.sh" << 'EOFBUILD'
#!/bin/bash
# Build and install AMDGPU_Abstracted with Mesa integration
# Supports Haiku and Linux
# Follows the same pattern as haiku-nvidia

set -e

baseDir="$PWD"

# Detect OS
OS_NAME=$(uname -s)
if command -v getarch &> /dev/null; then
    # Running on Haiku
    ARCH="$(getarch)"
    ON_HAIKU=true
else
    # Running on Linux or other Unix-like
    ARCH="$(uname -m)"
    ON_HAIKU=false
fi

buildBaseDir="$PWD/build.$ARCH"
installDir="$PWD/install.$ARCH"

function log_info() {
    echo "[INFO] $*"
}

function log_ok() {
    echo "[✓] $*"
}

function log_error() {
    echo "[✗] $*" >&2
}

# Create install directory
mkdir -p "$installDir/develop/lib/pkgconfig"

# Build AMDGPU_Abstracted core first
log_info "Building AMDGPU_Abstracted core libraries..."

cd "$baseDir"

buildDir="$baseDir/builddir_AMDGPU_Abstracted"

if [ -d "$buildDir" ]; then
    rm -rf "$buildDir"
fi

meson setup "$buildDir" -Dprefix="$installDir"
ninja -C "$buildDir"
ninja -C "$buildDir" install

log_ok "AMDGPU_Abstracted built successfully"

# Build Haiku Accelerant
if [ "$ON_HAIKU" = true ]; then
    log_info "Building Haiku Accelerant Module..."
    
    cd "$baseDir/accelerant"
    
    accelBuildDir="$baseDir/builddir_accelerant"
    
    if [ -d "$accelBuildDir" ]; then
        rm -rf "$accelBuildDir"
    fi
    
    meson setup "$accelBuildDir" -Dprefix="$installDir"
    ninja -C "$accelBuildDir"
    ninja -C "$accelBuildDir" install
    
    log_ok "Accelerant module built successfully"
fi

# Build Mesa (Haiku only)
if [ "$ON_HAIKU" = true ]; then
    log_info "Building Mesa for Haiku..."
    
    cd "$baseDir"
    
    if [ ! -d "mesa_source/.git" ]; then
        log_info "Cloning Mesa repository..."
        git clone --depth 1 https://gitlab.freedesktop.org/mesa/mesa.git mesa_source
    fi
    
    buildDir="$baseDir/mesa_build"
    
    if [ -d "$buildDir" ]; then
        rm -rf "$buildDir"
    fi
    
    log_info "Configuring Mesa for Haiku..."
    
    # CRITICAL: Source directory MUST be the last argument to meson setup
    # CRITICAL: Do NOT pass options AFTER the source directory path
    
    meson setup "$buildDir" \
        -Dprefix="$installDir" \
        -Dbuildtype=release \
        -Doptimization=3 \
        -Dgallium-drivers= \
        -Dplatforms=haiku \
        -Dopengl=true \
        -Dglx=disabled \
        -Degl=disabled \
        -Dgles2=enabled \
        -Dshader-cache=enabled \
        -Dvulkan-drivers= \
        mesa_source
    
    ninja -C "$buildDir"
    ninja -C "$buildDir" install
    
    log_ok "Mesa built successfully for Haiku"
else
    log_info "Skipping Mesa build on Linux"
    log_info "Mesa for GPU acceleration requires Haiku system libraries"
    log_info "The AMDGPU_Abstracted core components are still available"
    log_info "To use GPU acceleration, run this build on Haiku OS"
fi

# Summary
echo ""
echo "════════════════════════════════════════════════════════════"
echo "Build complete!"
echo "════════════════════════════════════════════════════════════"
echo "Installation directory: $installDir"
echo ""
echo "Next steps:"
echo "  1. Deploy: ./scripts/deploy_haiku.sh"
echo "  2. Verify: ./scripts/verify_installation.sh /boot/home/config/non-packaged"
echo ""
EOFBUILD

chmod +x "$PROJECT_ROOT/AMDGPU_Abstracted/Build.sh"

echo "[✓] Build.sh fixed"
echo ""
echo "Changes made:"
echo "  • Correct meson setup syntax (source directory last)"
echo "  • Removed problematic Mesa options"
echo "  • Added clear comments about meson syntax"
echo "  • Simplified and streamlined build process"
echo ""
echo "Now run:"
echo "  cd AMDGPU_Abstracted"
echo "  rm -rf mesa_build builddir_mesa builddir_accelerant"
echo "  ./Build.sh"
echo ""

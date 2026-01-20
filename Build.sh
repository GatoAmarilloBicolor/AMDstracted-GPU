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

function buildProject {
    projectName="$1"
    shift
    buildDir="$buildBaseDir/$projectName"

    log_info "Building: $projectName"
    cd "$baseDir/source/$projectName" 2>/dev/null || cd "$baseDir/$projectName"

    meson setup "$buildDir" \
        -Dpkg_config_path="$installDir/develop/lib/pkgconfig" \
        -Dprefix="$installDir" "$@"

    ninja -C "$buildDir"
    ninja -C "$buildDir" install
    
    log_ok "$projectName built successfully"
}

function buildProjectInPlace {
    projectName="$1"
    shift
    buildDir="$baseDir/builddir_$projectName"

    log_info "Building: $projectName (in-place)"
    
    if [ -d "$buildDir" ]; then
        rm -rf "$buildDir"
    fi
    
    meson setup "$buildDir" \
        -Dprefix="$installDir" "$@"

    ninja -C "$buildDir"
    ninja -C "$buildDir" install
    
    log_ok "$projectName built successfully"
}

# Create install directory
mkdir -p "$installDir/develop/lib/pkgconfig"

# Build AMDGPU_Abstracted core first
log_info "Building AMDGPU_Abstracted core libraries..."
buildProjectInPlace AMDGPU_Abstracted

# Build Mesa with R600 driver (optional, for full GPU acceleration)
log_info "Mesa GPU acceleration is optional"
log_info "The Accelerant module is the primary component and is already built"

if [ "$ON_HAIKU" = true ]; then
    log_info ""
    log_info "To build Mesa with R600 GPU driver on Haiku, use:"
    log_info "  ./build_mesa.sh"
    log_info ""
    log_info "Mesa is NOT built by Build.sh due to Meson compatibility on Haiku"
    log_info "Use build_mesa.sh for independent Mesa compilation"
else
    log_info "Skipping Mesa build on Linux"
    log_info "Mesa requires Haiku system libraries"
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
echo "  1. Review installed files in: $installDir"
echo "  2. Deploy to Haiku system using deployment script"
echo "  3. Configure environment:"
echo "     export LD_LIBRARY_PATH=$installDir/lib:\$LD_LIBRARY_PATH"
echo "     export PKG_CONFIG_PATH=$installDir/lib/pkgconfig:\$PKG_CONFIG_PATH"
echo ""

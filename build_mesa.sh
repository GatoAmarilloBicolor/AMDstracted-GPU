#!/bin/bash
# Build Mesa for Haiku with AMD GPU driver support
# Usage: ./build_mesa.sh [driver]
# Examples:
#   ./build_mesa.sh          # Builds with r600 (default)
#   ./build_mesa.sh r600     # Explicitly use r600
#   ./build_mesa.sh radeonsi # Use radeonsi for GCN+ GPUs

set -e

baseDir="$PWD"
DRIVER="${1:-r600}"  # Default to r600 if not specified

# Detect architecture
if command -v getarch &> /dev/null; then
    ARCH="$(getarch)"
    ON_HAIKU=true
else
    ARCH="$(uname -m)"
    ON_HAIKU=false
fi

installDir="$baseDir/install.$ARCH"
buildDir="$baseDir/builddir_mesa"

echo "════════════════════════════════════════════════════════════"
echo "Mesa Build for AMD GPU - Haiku OS"
echo "════════════════════════════════════════════════════════════"
echo ""
echo "Architecture: $ARCH"
echo "GPU Driver:  $DRIVER"
echo "Build dir:   $buildDir"
echo "Install dir: $installDir"
echo ""

if [ "$ON_HAIKU" != true ]; then
    echo "[✗] This script must be run on Haiku OS"
    echo "    Current OS: $(uname -s)"
    exit 1
fi

# Validate driver
case "$DRIVER" in
    r300|r600|radeonsi)
        echo "[✓] Driver: $DRIVER (supported)"
        ;;
    *)
        echo "[✗] Unknown driver: $DRIVER"
        echo ""
        echo "Supported drivers:"
        echo "  - r300:     AMD Radeon (R300-R500, ancient)"
        echo "  - r600:     AMD Radeon (HD 2000-5000 series, legacy)"
        echo "  - radeonsi: AMD Radeon (GCN+, modern RX series)"
        exit 1
        ;;
esac

echo ""

# Clone Mesa if not present
if [ ! -d "mesa_source/.git" ]; then
    echo "[INFO] Cloning Mesa repository..."
    git clone --depth 1 https://gitlab.freedesktop.org/mesa/mesa.git mesa_source
    echo "[✓] Mesa cloned"
    echo ""
fi

# Clean previous build
if [ -d "$buildDir" ]; then
    echo "[INFO] Cleaning previous build..."
    rm -rf "$buildDir"
fi

mkdir -p "$installDir/develop/lib/pkgconfig"

# Configure Mesa
echo "[INFO] Configuring Mesa with $DRIVER driver..."
echo ""

# Enter mesa_source directory for meson
pushd "$baseDir/mesa_source" > /dev/null

# Use meson without specifying source dir (use current dir implicitly)
meson setup "$buildDir" \
    -Dprefix="$installDir" \
    -Dbuildtype=release \
    -Doptimization=3 \
    -Dgallium-drivers=$DRIVER \
    -Dplatforms=haiku \
    -Dopengl=true \
    -Dglx=disabled \
    -Degl=disabled \
    -Dgles2=enabled \
    -Dshader-cache=enabled \
    -Dvulkan-drivers= \
    -Dllvm=disabled

popd > /dev/null

echo ""
echo "[INFO] Building Mesa..."
echo ""

ninja -C "$buildDir"

echo ""
echo "[INFO] Installing Mesa..."
echo ""

ninja -C "$buildDir" install

echo ""
echo "════════════════════════════════════════════════════════════"
echo "[✓] Mesa built successfully!"
echo "════════════════════════════════════════════════════════════"
echo ""
echo "Driver:  $DRIVER"
echo "Install: $installDir"
echo ""
echo "Next steps:"
echo "  1. Test Mesa: glinfo (if available)"
echo "  2. Configure environment variables:"
echo "     export LD_LIBRARY_PATH=$installDir/lib:\$LD_LIBRARY_PATH"
echo "     export PKG_CONFIG_PATH=$installDir/lib/pkgconfig:\$PKG_CONFIG_PATH"
echo "  3. Restart graphics server: pkill -9 app_server"
echo ""
echo "To build with a different driver:"
echo "  ./build_mesa.sh radeonsi"
echo "  ./build_mesa.sh r300"
echo ""

#!/bin/bash
# Simplified Mesa build for Haiku - avoids meson syntax issues
# This version uses a more direct approach

set -e

baseDir="$PWD"
DRIVER="${1:-r600}"

# Detect if on Haiku
if ! command -v getarch &> /dev/null; then
    echo "[✗] This script must run on Haiku OS"
    exit 1
fi

ARCH="$(getarch)"
buildDir="$baseDir/builddir_mesa"
installDir="$baseDir/install.$ARCH"

echo "════════════════════════════════════════════════════════════"
echo "Mesa Build - Haiku (Simplified)"
echo "════════════════════════════════════════════════════════════"
echo ""
echo "Driver: $DRIVER"
echo ""

# Clone if needed
if [ ! -d "mesa_source" ]; then
    echo "[INFO] Cloning Mesa..."
    git clone --depth 1 https://gitlab.freedesktop.org/mesa/mesa.git mesa_source
fi

# Clean
if [ -d "$buildDir" ]; then
    rm -rf "$buildDir"
fi

mkdir -p "$buildDir"
mkdir -p "$installDir/lib/pkgconfig"

# Go to mesa source
cd "$baseDir/mesa_source"

echo "[INFO] Running meson setup from mesa_source directory..."
echo ""

# Try the most basic meson setup
# This should work: meson setup <builddir> <options>
# The builddir is relative to where we are now
meson setup "$buildDir" \
    -Dprefix="$installDir" \
    -Dbuildtype=release \
    -Dgallium-drivers=$DRIVER \
    -Dplatforms=haiku || {
    
    echo ""
    echo "[✗] Meson setup failed"
    echo ""
    echo "Debugging info:"
    echo "  Current dir: $(pwd)"
    echo "  Build dir: $buildDir"
    echo "  Meson version: $(meson --version)"
    echo ""
    echo "Try these commands manually:"
    echo "  cd $baseDir/mesa_source"
    echo "  meson setup $buildDir -Dprefix=$installDir -Dgallium-drivers=$DRIVER -Dplatforms=haiku"
    exit 1
}

echo ""
echo "[INFO] Building..."
ninja -C "$buildDir" || {
    echo "[✗] Build failed"
    exit 1
}

echo ""
echo "[INFO] Installing..."
ninja -C "$buildDir" install || {
    echo "[✗] Install failed"
    exit 1
}

cd "$baseDir"

echo ""
echo "════════════════════════════════════════════════════════════"
echo "[✓] Mesa built successfully!"
echo "════════════════════════════════════════════════════════════"
echo ""
echo "Next:"
echo "  sudo ./install_haiku.sh"
echo ""

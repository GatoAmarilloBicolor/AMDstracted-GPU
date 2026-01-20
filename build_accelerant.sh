#!/bin/bash
# Build script for AMD Accelerant (platform-agnostic)
# Compiles the refactored accelerant with platform abstraction layer

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ACCELERANT_DIR="$SCRIPT_DIR/accelerant"
BUILD_DIR="$ACCELERANT_DIR/builddir_accelerant"
INSTALL_DIR="$SCRIPT_DIR/install.x86_64"

echo "════════════════════════════════════════════════════════"
echo "AMD Accelerant Build (Platform-Agnostic)"
echo "════════════════════════════════════════════════════════"
echo ""
echo "Source:    $ACCELERANT_DIR"
echo "Build:     $BUILD_DIR"
echo "Install:   $INSTALL_DIR"
echo ""

# Detect OS
OS=$(uname -s)
echo "[INFO] Detected OS: $OS"

# Clean previous build
if [ -d "$BUILD_DIR" ]; then
    echo "[INFO] Cleaning previous build..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"

# Configure with Meson
echo "[INFO] Configuring Meson..."
cd "$ACCELERANT_DIR"

if [ "$OS" = "Haiku" ]; then
    echo "[INFO] Building for Haiku OS"
    meson setup "$BUILD_DIR" \
        -Dprefix="$INSTALL_DIR" \
        --buildtype=debugoptimized \
        -Dwarning_level=3
else
    echo "[INFO] Building for $OS (validation mode)"
    meson setup "$BUILD_DIR" \
        --buildtype=debugoptimized \
        -Dwarning_level=3
fi

# Build
echo "[INFO] Building..."
cd "$BUILD_DIR"
ninja -j$(nproc)

# Install (Haiku only)
if [ "$OS" = "Haiku" ]; then
    echo "[INFO] Installing..."
    ninja install
    echo ""
    echo "✓ Accelerant installed successfully"
    echo "  Location: /boot/system/add-ons/accelerants/"
    echo ""
    echo "Next: Restart graphics server or reboot system"
else
    echo ""
    echo "✓ Accelerant compiled successfully"
    echo "  Library: $BUILD_DIR/libamd_accelerant_core.a"
    echo ""
    echo "[INFO] To use on Haiku:"
    echo "  1. Copy to Haiku system"
    echo "  2. Run: ninja install"
    echo "  3. Restart graphics server"
fi

echo ""
echo "════════════════════════════════════════════════════════"
echo "Build complete!"
echo "════════════════════════════════════════════════════════"

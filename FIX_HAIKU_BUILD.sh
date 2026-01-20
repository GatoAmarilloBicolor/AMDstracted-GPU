#!/bin/bash
# Fix script for Haiku OS - Clean build of refactored accelerant

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ACCELERANT_DIR="$SCRIPT_DIR/accelerant"
BUILD_DIR="$ACCELERANT_DIR/builddir_accelerant_fixed"
INSTALL_DIR="$SCRIPT_DIR/install.x86_64"

echo "════════════════════════════════════════════════════════"
echo "AMD Accelerant - HAIKU FIX & REBUILD"
echo "════════════════════════════════════════════════════════"
echo ""
echo "This script fixes stale build configurations on Haiku"
echo ""

# Step 1: Verify we're on Haiku
if [ "$(uname)" != "Haiku" ]; then
    echo "❌ ERROR: This script must be run on Haiku OS"
    echo "Current OS: $(uname)"
    exit 1
fi

echo "✓ Running on Haiku OS"
echo ""

# Step 2: Clean old build directories
echo "[Step 1/4] Cleaning old build directories..."
rm -rf "$ACCELERANT_DIR/builddir_accelerant"
rm -rf "$ACCELERANT_DIR/builddir_accelerant_fixed"
rm -rf "$ACCELERANT_DIR/.build"
echo "✓ Cleaned"
echo ""

# Step 3: Verify accelerant_api.h and accelerant_haiku.h exist
echo "[Step 2/4] Verifying platform abstraction headers..."
if [ ! -f "$ACCELERANT_DIR/include/accelerant_api.h" ]; then
    echo "❌ ERROR: accelerant_api.h not found"
    echo "   Expected at: $ACCELERANT_DIR/include/accelerant_api.h"
    exit 1
fi

if [ ! -f "$ACCELERANT_DIR/include/accelerant_haiku.h" ]; then
    echo "❌ ERROR: accelerant_haiku.h not found"
    echo "   Expected at: $ACCELERANT_DIR/include/accelerant_haiku.h"
    exit 1
fi

if [ ! -f "$ACCELERANT_DIR/src/Accelerant_v2.c" ]; then
    echo "❌ ERROR: Accelerant_v2.c not found"
    echo "   Expected at: $ACCELERANT_DIR/src/Accelerant_v2.c"
    exit 1
fi

echo "✓ Headers verified:"
echo "  - accelerant_api.h"
echo "  - accelerant_haiku.h"  
echo "  - Accelerant_v2.c"
echo ""

# Step 4: Configure with Meson
echo "[Step 3/4] Configuring Meson (fresh build)..."
cd "$ACCELERANT_DIR"

meson setup "$BUILD_DIR" \
    -Dprefix="$INSTALL_DIR" \
    --buildtype=debugoptimized \
    -Dwarning_level=3

echo "✓ Meson configured"
echo ""

# Step 5: Build
echo "[Step 4/4] Building accelerant..."
cd "$BUILD_DIR"
ninja -j$(nproc)

echo ""
echo "════════════════════════════════════════════════════════"
echo "Build successful! Installing..."
echo "════════════════════════════════════════════════════════"
echo ""

ninja install

echo ""
echo "════════════════════════════════════════════════════════"
echo "✓ AMD Accelerant installed successfully"
echo "════════════════════════════════════════════════════════"
echo ""
echo "Installation location:"
echo "  /boot/system/add-ons/accelerants/amd_gfx.accelerant"
echo ""
echo "Next steps:"
echo "  1. Restart Haiku (or just restart app_server):"
echo "     $ pkill -9 app_server"
echo "     (Haiku will auto-restart it)"
echo ""
echo "  2. Test graphics acceleration:"
echo "     - Run graphics-intensive applications"
echo "     - Check GPU utilization"
echo ""
echo "════════════════════════════════════════════════════════"

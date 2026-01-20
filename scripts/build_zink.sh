#!/bin/bash
# Build Zink (OpenGL over Vulkan) for Haiku
# This script compiles only the necessary Mesa components

set -e

echo "🌋 Building Zink + RADV for Haiku..."

# Clone Mesa if not present
if [ ! -d "mesa" ]; then
    echo "[1/4] Cloning Mesa..."
    git clone --depth 1 https://gitlab.freedesktop.org/mesa/mesa.git
else
    echo "[1/4] Mesa already cloned"
fi

cd mesa

# Clean previous build
rm -rf build

echo "[2/4] Configuring Mesa build..."
meson setup build \
    --prefix=/boot/home/config/non-packaged \
    -Dvulkan-drivers=amd \
    -Dgallium-drivers=zink \
    -Dglx=disabled \
    -Degl=enabled \
    -Dgles1=disabled \
    -Dgles2=enabled \
    -Dshared-glapi=disabled \
    -Dbuildtype=release

echo "[3/4] Building Mesa (this takes ~10 minutes)..."
ninja -C build

echo "[4/4] Installing Mesa libraries..."
ninja -C build install

cd ..

echo ""
echo "✅ Zink + RADV installed successfully!"
echo ""
echo "Libraries installed to:"
echo "  /boot/home/config/non-packaged/lib/libGL.so (Zink)"
echo "  /boot/home/config/non-packaged/lib/dri/zink_dri.so"
echo "  /boot/home/config/non-packaged/lib/libvulkan_radeon.so (RADV)"
echo ""
echo "To use:"
echo "  source env_radv.sh"
echo "  GLInfo"

#!/bin/bash
# Setup Mesa RADV and Zink for AMDGPU_Abstracted
# This script clones Mesa and configures it to use our DRM shim

set -e

echo "🌋 Mesa RADV/Zink Setup for AMDGPU_Abstracted"
echo "=============================================="

# 1. Clone Mesa if not present
if [ ! -d "mesa" ]; then
    echo "[1/5] Cloning Mesa (this may take a while)..."
    git clone --depth 1 https://gitlab.freedesktop.org/mesa/mesa.git
    cd mesa
    git checkout main
    cd ..
else
    echo "[1/5] Mesa already cloned, skipping..."
fi

# 2. Build our DRM shim first
echo "[2/5] Building DRM Shim..."
make drm-shim

# 3. Configure Mesa with RADV and Zink
echo "[3/5] Configuring Mesa build..."
cd mesa

# Create build directory
mkdir -p build

# Configure with Meson
meson setup build \
    --prefix=/boot/home/config/non-packaged \
    -Dvulkan-drivers=amd \
    -Dgallium-drivers=zink \
    -Dplatforms=haiku \
    -Ddri-drivers= \
    -Dglx=disabled \
    -Degl=enabled \
    -Dgles1=disabled \
    -Dgles2=enabled \
    -Dshared-glapi=enabled \
    -Dbuildtype=release

echo "[4/5] Building Mesa (this will take several minutes)..."
ninja -C build

echo "[5/5] Installing Mesa..."
ninja -C build install

cd ..

echo ""
echo "✅ Mesa RADV/Zink installation complete!"
echo ""
echo "Next steps:"
echo "1. Set environment variables:"
echo "   export LD_LIBRARY_PATH=../drm-shim:\$LD_LIBRARY_PATH"
echo "   export VK_ICD_FILENAMES=/boot/home/config/non-packaged/share/vulkan/icd.d/radeon_icd.x86_64.json"
echo ""
echo "2. Test with: vulkaninfo | grep AMDGPU"
echo "3. Test OpenGL via Zink: GLInfo"

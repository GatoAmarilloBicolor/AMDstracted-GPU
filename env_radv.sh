#!/bin/bash
# Environment Configuration for RADV/Zink Integration
# Source this file before running applications that need GPU acceleration

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "🌋 Configuring RADV/Zink Environment..."

# 1. Force our DRM shim to be loaded instead of system libdrm
export LD_PRELOAD="$SCRIPT_DIR/drm-shim/libdrm_amdgpu.so:$LD_PRELOAD"
echo "   ✓ DRM Shim: $SCRIPT_DIR/drm-shim/libdrm_amdgpu.so"

# 2. Point Vulkan loader to our RADV ICD
if [ -f "/boot/home/config/non-packaged/share/vulkan/icd.d/radeon_icd.x86_64.json" ]; then
    export VK_ICD_FILENAMES="/boot/home/config/non-packaged/share/vulkan/icd.d/radeon_icd.x86_64.json"
    echo "   ✓ Vulkan ICD: RADV (AMD)"
elif [ -f "/boot/home/config/non-packaged/share/vulkan/icd.d/radeon_icd.json" ]; then
    export VK_ICD_FILENAMES="/boot/home/config/non-packaged/share/vulkan/icd.d/radeon_icd.json"
    echo "   ✓ Vulkan ICD: RADV (AMD)"
else
    echo "   ⚠ Vulkan ICD not found (Mesa not installed yet?)"
fi

# 3. Add Mesa libraries to library path
export LD_LIBRARY_PATH="/boot/home/config/non-packaged/lib:$LD_LIBRARY_PATH"

# 4. Point OpenGL to Zink (OpenGL over Vulkan)
export LIBGL_DRIVERS_PATH="/boot/home/config/non-packaged/lib/dri"
export MESA_LOADER_DRIVER_OVERRIDE="zink"
echo "   ✓ OpenGL: Zink (GL over Vulkan)"

# 5. Enable Mesa debug output (optional, comment out for production)
# export MESA_DEBUG=1
# export LIBGL_DEBUG=verbose

# 6. Force Vulkan to use our device
export VK_DRIVER_FILES="/boot/home/config/non-packaged/share/vulkan/icd.d/radeon_icd*.json"

# 7. Ensure rmapi_server is running
if ! pgrep -x "rmapi_server" > /dev/null; then
    echo "   ⚠ rmapi_server not running! Start it with: ./rmapi_server &"
else
    echo "   ✓ rmapi_server is running"
fi

echo ""
echo "Environment configured! You can now run:"
echo "  - vulkaninfo          (to test Vulkan)"
echo "  - GLInfo              (to test OpenGL via Zink)"
echo "  - glxgears            (to test rendering)"
echo ""

#!/bin/bash

# Enable OpenGL with Zink for AMDGPU_Abstracted on Haiku OS
# Uses Vulkan backend (RADV) to implement OpenGL

echo "🎨 Enabling OpenGL with Zink on Haiku OS"
echo "=========================================================="

# Check if we're on Haiku
OS="$(uname -s)"
if [ "$OS" != "Haiku" ]; then
    echo "❌ This script is for Haiku OS only. Current OS: $OS"
    exit 1
fi

# Mesa and Vulkan paths on Haiku
MESA_PREFIX="/boot/home/config/non-packaged"
DRI_PATH="$MESA_PREFIX/lib/dri"
VULKAN_PATH="$MESA_PREFIX/share/vulkan/icd.d"

# Step 1: Configure environment
echo "[1/4] Configuring OpenGL environment..."

# OpenGL driver search path
export LIBGL_DRIVERS_PATH="$DRI_PATH"
echo "   ✓ LIBGL_DRIVERS_PATH=$DRI_PATH"

# Force Zink (OpenGL over Vulkan)
export MESA_LOADER_DRIVER_OVERRIDE="zink"
echo "   ✓ MESA_LOADER_DRIVER_OVERRIDE=zink"

# Vulkan ICD configuration
export VK_ICD_FILENAMES="$VULKAN_PATH/radeon_icd.x86_64.json"
export VK_DRIVER_FILES="$VULKAN_PATH/radeon_icd*.json"
echo "   ✓ VK_ICD_FILENAMES=$VK_ICD_FILENAMES"

# Library paths
export LD_LIBRARY_PATH="$MESA_PREFIX/lib:$LD_LIBRARY_PATH"
export LIBRARY_PATH="$MESA_PREFIX/lib:$LIBRARY_PATH"
echo "   ✓ Library paths configured"

# Optional: Enable debug output
export LIBGL_DEBUG="verbose"
export MESA_DEBUG="verbose"
export RADV_DEBUG="all"
export VK_LOADER_DEBUG="error"

# Step 2: Check RMAPI server
echo "[2/4] Checking RMAPI server..."
if ! ps aux | grep -q "[r]mapi_server"; then
    echo "   ⚠ rmapi_server not running!"
    echo "   Start it with: ./build/rmapi_server &"
    exit 1
else
    echo "   ✓ rmapi_server is running"
fi

# Step 3: Verify Mesa installation
echo "[3/4] Verifying Mesa installation..."
if [ ! -d "$DRI_PATH" ]; then
    echo "   ❌ DRI drivers not found at $DRI_PATH"
    echo "   Install Mesa: pkgman install mesa_devel"
    exit 1
fi
echo "   ✓ DRI drivers found"

if [ ! -f "$VK_ICD_FILENAMES" ]; then
    echo "   ❌ Vulkan ICD not found at $VK_ICD_FILENAMES"
    echo "   Make sure RADV ICD is installed"
    exit 1
fi
echo "   ✓ Vulkan ICD found"

# Step 4: Test OpenGL
echo "[4/4] Testing OpenGL..."
if command -v glxinfo &> /dev/null; then
    glxinfo -B 2>&1 | head -20
    echo "   ✓ OpenGL test complete"
elif command -v GLInfo &> /dev/null; then
    GLInfo 2>&1 | head -20
    echo "   ✓ OpenGL test complete"
else
    echo "   ⚠ Neither glxinfo nor GLInfo found"
fi

echo ""
echo "✅ OpenGL with Zink configured for Haiku OS!"
echo ""
echo "Configuration summary:"
echo "  - OpenGL: Zink (GL over Vulkan)"
echo "  - Vulkan Backend: RADV (AMD)"
echo "  - DRI Drivers: $DRI_PATH"
echo "  - Vulkan ICD: $VK_ICD_FILENAMES"
echo ""
echo "Test with: glxinfo, GLInfo, or glxgears"
echo ""
echo "Environment variables are now set. Run your OpenGL applications!"

#!/bin/bash

# Enable Vulkan with RADV for AMDGPU_Abstracted
# Based on Haiku-nvidia NVK approach, adapted for AMD RADV

echo "🚀 Enabling Vulkan support with RADV (real hardware from userland)"
echo "=========================================================="

# Paths
LOCAL_PREFIX=/home/fenux/local
DRM_SHIM_PATH=/home/fenux/src/project_amdbstraction/AMDGPU_Abstracted/src/drm

# Set library paths
export LD_LIBRARY_PATH=$LOCAL_PREFIX/lib:$DRM_SHIM_PATH:$LD_LIBRARY_PATH

# Set Vulkan ICD
export VK_ICD_FILENAMES=$LOCAL_PREFIX/share/vulkan/icd.d/radeon_icd.x86_64.json

# Debugging
export VK_LOADER_DEBUG=all
export RADV_DEBUG=all

# Start RMAPI server if not running
if ! ps aux | grep -q "[r]mapi_server"; then
    echo "[1/3] Starting RMAPI server..."
    ./build/rmapi_server &
    sleep 2
else
    echo "[1/3] RMAPI server already running"
fi

echo "[2/3] Environment configured:"
echo "  LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
echo "  VK_ICD_FILENAMES=$VK_ICD_FILENAMES"

echo "[3/3] Testing Vulkan..."
vulkaninfo | head -20

echo ""
echo "✅ Vulkan enabled with RADV!"
echo "  Real hardware acceleration via userland RMAPI (no virtualization)"
echo ""
echo "To use in apps: set the same environment variables"
#!/bin/bash

# Test.sh - OS-Agnostic Test Script for AMDGPU_Abstracted
# Works on Haiku and Linux equally well

echo "🚀 Starting AMDGPU_Abstracted Tests (OS-Agnostic)..."

# 1. Detect OS
OS=$(uname -s | tr '[:upper:]' '[:lower:]')
echo "🌍 Detected OS: $OS"

# 2. Set OS-specific paths/tools
if [ "$OS" = "haiku" ]; then
    GL_TOOL="GLInfo"  # Haiku's OpenGL info
    DISPLAY_TOOL="Screen"  # Haiku display modes
    LIB_PATH="/boot/home/config/non-packaged/lib"
elif [ "$OS" = "linux" ]; then
    GL_TOOL="glxinfo"  # Linux GLX info
    DISPLAY_TOOL="xrandr"  # Linux display
    LIB_PATH="/usr/local/lib"  # Adjust if needed
else
    echo "❌ Unsupported OS: $OS"
    exit 1
fi

# 3. Export paths for libs
export LD_LIBRARY_PATH="$LIB_PATH:$LD_LIBRARY_PATH"

# 4. Test RMAPI (common to both)
echo "🧪 Testing RMAPI Server/Client..."
./rmapi_server &
SERVER_PID=$!
sleep 2  # Wait for server
./rmapi_client_demo
kill $SERVER_PID

# 5. Test Graphics (OS-specific but equivalent)
echo "🎮 Testing Graphics Acceleration..."
if command -v $GL_TOOL &> /dev/null; then
    $GL_TOOL | head -20  # Show first 20 lines for comparison
else
    echo "⚠️ $GL_TOOL not found, skipping GL test"
fi

if command -v $DISPLAY_TOOL &> /dev/null; then
    $DISPLAY_TOOL | head -10
else
    echo "⚠️ $DISPLAY_TOOL not found, skipping display test"
fi

# 6. Test Vulkan (common, if available)
if command -v vulkaninfo &> /dev/null; then
    echo "🔥 Testing Vulkan..."
    vulkaninfo --summary | head -15
else
    echo "⚠️ vulkaninfo not found"
fi

echo "✅ Tests completed. Check outputs for AMD hardware acceleration."
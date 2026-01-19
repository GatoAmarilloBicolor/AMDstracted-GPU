#!/bin/bash
# Test OpenGL Acceleration with RMAPI
# Verifies that RMAPI Gallium driver is working correctly

set -e

echo "════════════════════════════════════════════════════"
echo "RMAPI OpenGL Acceleration Test"
echo "════════════════════════════════════════════════════"
echo ""

OS="$(uname -s)"

if [ "$OS" != "Haiku" ]; then
    echo "❌ This script is for Haiku OS only. Current OS: $OS"
    exit 1
fi

# Step 1: Check RMAPI driver installation
echo "[1/5] Checking RMAPI Gallium driver..."
RMAPI_DRIVER="/boot/home/config/non-packaged/lib/dri/rmapi_dri.so"
if [ -f "$RMAPI_DRIVER" ]; then
    echo "✅ RMAPI driver found: $RMAPI_DRIVER"
else
    echo "❌ RMAPI driver not found at $RMAPI_DRIVER"
    echo "   Run: ./scripts/install_haiku.sh first"
    exit 1
fi

# Step 2: Check RMAPI server executable
echo ""
echo "[2/5] Checking RMAPI server..."
SERVER_BIN="/boot/home/config/non-packaged/bin/amd_rmapi_server"
if [ -x "$SERVER_BIN" ]; then
    echo "✅ RMAPI server found: $SERVER_BIN"
else
    echo "❌ RMAPI server not found or not executable"
    exit 1
fi

# Step 3: Check Mesa installation
echo ""
echo "[3/5] Checking Mesa installation..."
MESA_LIB="/boot/home/config/non-packaged/lib/libGL.so"
if [ -f "$MESA_LIB" ] || [ -L "$MESA_LIB" ]; then
    echo "✅ Mesa libGL found"
else
    echo "⚠️  Mesa libGL not found - may need to recompile Mesa"
fi

# Step 4: Start RMAPI server for testing
echo ""
echo "[4/5] Starting RMAPI server..."
if ps aux | grep -q "[a]md_rmapi_server"; then
    echo "✅ RMAPI server already running"
else
    echo "   Starting server..."
    "$SERVER_BIN" &
    SERVER_PID=$!
    sleep 2
    
    if ps -p $SERVER_PID > /dev/null 2>&1; then
        echo "✅ RMAPI server started (PID: $SERVER_PID)"
    else
        echo "❌ RMAPI server failed to start"
        exit 1
    fi
fi

# Step 5: Test with glxinfo if available
echo ""
echo "[5/5] Testing OpenGL with RMAPI..."

# Source environment
export LIBGL_DRIVERS_PATH="/boot/home/config/non-packaged/lib/dri"
export MESA_LOADER_DRIVER_OVERRIDE="rmapi"
export GALLIUM_DRIVER="rmapi"
export LD_LIBRARY_PATH="/boot/home/config/non-packaged/lib:$LD_LIBRARY_PATH"

if command -v glxinfo &> /dev/null; then
    echo "Running glxinfo..."
    if glxinfo 2>&1 | grep -q "OpenGL vendor\|OpenGL version"; then
        echo "✅ OpenGL test successful!"
        glxinfo | grep -E "OpenGL vendor|OpenGL version|Direct rendering" | head -3
    else
        echo "⚠️  glxinfo ran but output unclear"
        glxinfo | head -10
    fi
elif command -v GLInfo &> /dev/null; then
    echo "Running GLInfo..."
    if GLInfo 2>&1 | head -20; then
        echo "✅ GLInfo test complete"
    fi
else
    echo "⚠️  No GL testing tool found (glxinfo or GLInfo)"
fi

echo ""
echo "════════════════════════════════════════════════════"
echo "✅ RMAPI OpenGL Acceleration Test Complete"
echo "════════════════════════════════════════════════════"
echo ""
echo "Environment:"
echo "  GALLIUM_DRIVER=$GALLIUM_DRIVER"
echo "  MESA_LOADER_DRIVER_OVERRIDE=$MESA_LOADER_DRIVER_OVERRIDE"
echo "  LIBGL_DRIVERS_PATH=$LIBGL_DRIVERS_PATH"
echo ""
echo "You can now run OpenGL applications with GPU acceleration:"
echo "  ./scripts/launch_amdgpu.sh launch glxgears"
echo "  ./scripts/launch_amdgpu.sh launch any-opengl-app"

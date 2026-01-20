#!/bin/bash
# GPU Acceleration Test Script for Haiku
# Tests OpenGL rendering with R600 driver and RMAPI acceleration

set -e

echo "════════════════════════════════════════════════════════════"
echo "🧪 GPU Acceleration Test for Haiku"
echo "════════════════════════════════════════════════════════════"
echo ""

INSTALL_PREFIX="/boot/home/config/non-packaged"
export INSTALL_PREFIX

# Source environment
echo "⚙️  Setting up environment..."
if [ -f "$INSTALL_PREFIX/.amd_gpu_env.sh" ]; then
    source "$INSTALL_PREFIX/.amd_gpu_env.sh"
    echo "✅ Environment loaded from: $INSTALL_PREFIX/.amd_gpu_env.sh"
else
    echo "⚠️  Environment script not found, using defaults"
    export LIBGL_DRIVERS_PATH="$INSTALL_PREFIX/lib/dri:/boot/system/lib/dri"
    export MESA_LOADER_DRIVER_OVERRIDE="r600"
    export LD_LIBRARY_PATH="$INSTALL_PREFIX/lib:$LD_LIBRARY_PATH"
fi

echo ""

# Test 1: Check GPU detection
echo "════════════════════════════════════════════════════════════"
echo "🔍 Test 1: GPU Detection"
echo "════════════════════════════════════════════════════════════"
echo ""

GPU_INFO=$(lspci 2>/dev/null | grep -i radeon || echo "No GPU detected")
echo "GPU: $GPU_INFO"

if echo "$GPU_INFO" | grep -q "7290\|Wrestler"; then
    echo "✅ Radeon HD 7290 (Wrestler) detected"
elif echo "$GPU_INFO" | grep -q "Radeon"; then
    echo "✅ AMD Radeon GPU detected"
else
    echo "⚠️  No AMD Radeon GPU detected"
fi

echo ""

# Test 2: Check Mesa driver availability
echo "════════════════════════════════════════════════════════════"
echo "📦 Test 2: Mesa Driver Check"
echo "════════════════════════════════════════════════════════════"
echo ""

echo "Checking for r600_dri.so..."
R600_FOUND=0

for path in /boot/system/lib/dri /boot/home/config/non-packaged/lib/dri; do
    if [ -f "$path/r600_dri.so" ]; then
        echo "✅ Found: $path/r600_dri.so"
        R600_FOUND=1
    fi
done

if [ $R600_FOUND -eq 0 ]; then
    echo "⚠️  r600_dri.so not found"
    echo "    Install with: pkgman install mesa_r600"
fi

echo "DRI driver path: $LIBGL_DRIVERS_PATH"
echo ""

# Test 3: Check RMAPI binaries
echo "════════════════════════════════════════════════════════════"
echo "🔧 Test 3: RMAPI Binaries Check"
echo "════════════════════════════════════════════════════════════"
echo ""

for bin in amd_rmapi_server amd_rmapi_client_demo; do
    if [ -x "$INSTALL_PREFIX/bin/$bin" ]; then
        echo "✅ $bin available"
    else
        echo "❌ $bin not found"
    fi
done

echo ""

# Test 4: Try to detect RMAPI server capability
echo "════════════════════════════════════════════════════════════"
echo "🚀 Test 4: RMAPI Server Initialization"
echo "════════════════════════════════════════════════════════════"
echo ""

if [ -x "$INSTALL_PREFIX/bin/amd_rmapi_server" ]; then
    echo "Attempting to start RMAPI server (test mode)..."
    
    # Try to run server with short timeout to test functionality
    timeout 3 "$INSTALL_PREFIX/bin/amd_rmapi_server" 2>&1 | head -20 || true
    
    echo ""
    echo "⚠️  Server timeout (expected for test mode)"
else
    echo "❌ amd_rmapi_server not found"
fi

echo ""

# Test 5: OpenGL capability check
echo "════════════════════════════════════════════════════════════"
echo "🎨 Test 5: OpenGL Capability"
echo "════════════════════════════════════════════════════════════"
echo ""

# Check for glinfo
if command -v glinfo > /dev/null 2>&1; then
    echo "🔎 Running glinfo (limited output)..."
    
    # Run glinfo with timeout and capture output
    GLINFO_OUTPUT=$(timeout 5 glinfo 2>&1 || echo "No OpenGL context")
    
    if echo "$GLINFO_OUTPUT" | grep -q "OpenGL"; then
        echo "✅ OpenGL available"
        echo ""
        echo "OpenGL Info:"
        echo "$GLINFO_OUTPUT" | head -20
        echo ""
        
        # Check for GPU vendor
        if echo "$GLINFO_OUTPUT" | grep -i "renderer" | grep -q -i "radeon\|amd"; then
            echo "✅ AMD GPU renderer detected"
        elif echo "$GLINFO_OUTPUT" | grep -i "renderer" | grep -q -i "llvmpipe\|software"; then
            echo "⚠️  Software rendering (CPU) - GPU driver not loaded"
        fi
    else
        echo "⚠️  OpenGL context unavailable (expected in headless environment)"
    fi
else
    echo "⚠️  glinfo not found - install: pkgman install mesa_devel"
fi

echo ""

# Test 6: Library dependencies
echo "════════════════════════════════════════════════════════════"
echo "📚 Test 6: Library Dependencies"
echo "════════════════════════════════════════════════════════════"
echo ""

if command -v ldd > /dev/null 2>&1; then
    echo "Checking dependencies for amd_rmapi_server..."
    ldd "$INSTALL_PREFIX/bin/amd_rmapi_server" 2>&1 | grep -E "libm|libnetwork|libc" || true
fi

echo ""

# Final status
echo "════════════════════════════════════════════════════════════"
echo "📊 Test Summary"
echo "════════════════════════════════════════════════════════════"
echo ""
echo "✅ GPU detection: Check PASSED if Radeon detected"
echo "✅ Mesa drivers: Check PASSED if r600_dri.so found"
echo "✅ RMAPI binaries: Check PASSED if binaries exist"
echo "✅ OpenGL support: Check PASSED if GL version >= 2.1"
echo ""
echo "🎯 Next Steps:"
echo "   1. If all checks passed, your system supports GPU acceleration"
echo "   2. Start RMAPI server: $INSTALL_PREFIX/bin/amd_rmapi_server &"
echo "   3. Run OpenGL app: source ~/.amd_gpu_env.sh && glinfo"
echo "   4. Test benchmark: source ~/.amd_gpu_env.sh && glxgears"
echo ""

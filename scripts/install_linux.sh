#!/bin/bash

# 🏁 HIT Edition: Professional Linux Installer
# This script builds and installs the driver for Linux systems.
# Includes: driver, shared library, tests, and examples
# Developed by: Haiku Imposible Team (HIT)

set -e  # Exit on error

echo "════════════════════════════════════════════════════════════════"
echo "🚀 HIT Linux Installation - Complete Build"
echo "════════════════════════════════════════════════════════════════"
echo ""

# Check prerequisites
echo "📋 Checking prerequisites..."
if ! command -v gcc &> /dev/null; then
    echo "❌ GCC not found. Install with: sudo apt install gcc build-essential"
    exit 1
fi
if ! command -v meson &> /dev/null; then
    echo "❌ Meson not found. Install with: pip install meson"
    exit 1
fi
if ! command -v ninja &> /dev/null; then
    echo "❌ Ninja not found. Install with: sudo apt install ninja-build"
    exit 1
fi
echo "✅ Prerequisites OK"
echo ""

# 1. Build main driver
echo "────────────────────────────────────────────────────────────────"
echo "📦 Step 1: Building Main Driver"
echo "────────────────────────────────────────────────────────────────"
meson setup builddir
if [ $? -ne 0 ]; then
    echo "❌ Meson setup failed!"
    exit 1
fi
meson compile -C builddir
if [ $? -ne 0 ]; then
    echo "❌ Driver build failed!"
    exit 1
fi
echo "✅ Driver built successfully"
echo ""

# 2. Build and run tests
echo "────────────────────────────────────────────────────────────────"
echo "🧪 Step 2: Building and Running Test Suite"
echo "────────────────────────────────────────────────────────────────"
meson test -C builddir
if [ $? -ne 0 ]; then
    echo "❌ Test build/run failed!"
    exit 1
fi
echo "✅ Tests built and run successfully"
echo ""

# 4. Install system-wide (optional, requires sudo)
echo "────────────────────────────────────────────────────────────────"
echo "📂 Step 4: System Installation (Optional)"
echo "────────────────────────────────────────────────────────────────"

INSTALL_DIR="/usr/local/bin"
LIB_DIR="/usr/local/lib"

# Ask for confirmation
read -p "Install driver system-wide? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Installing to system paths..."
    
    # Check for sudo access
    if ! sudo -n true 2>/dev/null; then
        echo "🔐 Sudo password required:"
        sudo true || { echo "❌ Sudo failed"; exit 1; }
    fi
    
    # Install main artifacts
    sudo cp -f builddir/rmapi_server "$INSTALL_DIR/amd_rmapi_server"
    sudo chmod +x "$INSTALL_DIR/amd_rmapi_server"

    sudo cp -f builddir/libamdgpu.so "$LIB_DIR/"
    sudo ldconfig

    cp -f builddir/rmapi_client_demo "$HOME/amd_rmapi_client_demo"
    chmod +x "$HOME/amd_rmapi_client_demo"

    # Install test suite
    sudo cp -f builddir/test_runner "$INSTALL_DIR/amd_test_suite"
    sudo chmod +x "$INSTALL_DIR/amd_test_suite"
    
    echo "✅ System installation complete"
    INSTALLED=1
else
    echo "⏭️  Skipping system installation"
    INSTALLED=0
fi
echo ""

# 5. Summary
echo "════════════════════════════════════════════════════════════════"
echo "✅ BUILD COMPLETE"
echo "════════════════════════════════════════════════════════════════"
echo ""
echo "📊 Build Summary:"
echo "  • Driver binary:      rmapi_server ✅"
echo "  • Shared library:     libamdgpu.so ✅"
echo "  • Test suite:         test_runner ✅"
echo "  • Client demo:        rmapi_client_demo ✅"
echo ""

if [ $INSTALLED -eq 1 ]; then
    echo "📍 System Paths (Installed):"
    echo "  • Brain:             $INSTALL_DIR/amd_rmapi_server"
    echo "  • Library:           $LIB_DIR/libamdgpu.so"
    echo "  • Test Suite:        $INSTALL_DIR/amd_test_suite"
    echo "  • Client Demo:       $HOME/amd_rmapi_client_demo"
    echo ""
    echo "🛠️  Quick Start (Installed):"
    echo "  1. Start server:     amd_rmapi_server &"
    echo "  2. Run client:       ~/amd_rmapi_client_demo"
    echo "  3. Run tests:        amd_test_suite"
else
    echo "📍 Local Paths (Not Installed):"
    echo "  • Brain:             ./builddir/rmapi_server"
    echo "  • Library:           ./builddir/libamdgpu.so"
    echo "  • Test Suite:        ./builddir/test_runner"
    echo "  • Client Demo:       ./builddir/rmapi_client_demo"
    echo ""
    echo "🛠️  Quick Start (Local):"
    echo "  1. Start server:     ./builddir/rmapi_server &"
    echo "  2. Run client:       ./builddir/rmapi_client_demo"
    echo "  3. Run tests:        ./builddir/test_runner"
fi

echo ""
echo "📚 Documentation:"
echo "  • README:             ./README.md"
echo "  • Architecture:       ./docs/ARCHITECTURE_STEP_BY_STEP.md"
echo "  • Master Guide:       ../MAESTRO.md"
echo "  • Tests Guide:        ./tests/README.md"
echo ""
echo "════════════════════════════════════════════════════════════════"
echo "🎉 SUCCESS - Powered by Haiku Imposible Team (HIT)"
echo "════════════════════════════════════════════════════════════════"

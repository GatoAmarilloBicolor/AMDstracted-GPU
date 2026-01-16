#!/bin/bash

# 🏁 HIT Edition: FreeBSD Installer
# Builds and installs the AMD driver for FreeBSD
# Includes: driver, shared library, and tests
# Developed by: Haiku Imposible Team (HIT)

set -e

echo "════════════════════════════════════════════════════════════════"
echo "🚀 HIT FreeBSD Installation - Complete Build"
echo "════════════════════════════════════════════════════════════════"
echo ""

# Check prerequisites
echo "📋 Checking prerequisites..."
if ! command -v gcc &> /dev/null && ! command -v clang &> /dev/null; then
    echo "❌ Compiler not found. Install with: pkg install gcc or pkg install clang"
    exit 1
fi
if ! command -v make &> /dev/null; then
    echo "❌ Make not found. Install with: pkg install gmake"
    exit 1
fi
echo "✅ Prerequisites OK"
echo ""

# Use gmake if available (FreeBSD convention)
MAKE_CMD=make
if command -v gmake &> /dev/null; then
    MAKE_CMD=gmake
fi

# 1. Build main driver for FreeBSD
echo "────────────────────────────────────────────────────────────────"
echo "📦 Step 1: Building Main Driver for FreeBSD"
echo "────────────────────────────────────────────────────────────────"
OS=freebsd USERLAND_MODE=1 $MAKE_CMD clean all
if [ $? -ne 0 ]; then
    echo "❌ Driver build failed!"
    exit 1
fi
echo "✅ Driver built successfully for FreeBSD"
echo ""

# 2. Build tests
echo "────────────────────────────────────────────────────────────────"
echo "🧪 Step 2: Building Test Suite"
echo "────────────────────────────────────────────────────────────────"
cd tests
$MAKE_CMD -f Makefile.test clean
$MAKE_CMD -f Makefile.test
if [ $? -ne 0 ]; then
    echo "❌ Test build failed!"
    exit 1
fi
echo "✅ Tests built successfully"
cd ..
echo ""

# 3. Run tests
echo "────────────────────────────────────────────────────────────────"
echo "🧪 Step 3: Running Test Suite"
echo "────────────────────────────────────────────────────────────────"
cd tests
./test_suite
TEST_RESULT=$?
cd ..
echo ""

# 4. Install to FreeBSD system paths
echo "────────────────────────────────────────────────────────────────"
echo "📂 Step 4: FreeBSD Installation"
echo "────────────────────────────────────────────────────────────────"

INSTALL_DIR="/usr/local/bin"
LIB_DIR="/usr/local/lib"

# Ask for confirmation (need sudo for system install)
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
    sudo cp -f rmapi_server "$INSTALL_DIR/amd_rmapi_server"
    sudo chmod +x "$INSTALL_DIR/amd_rmapi_server"
    
    sudo cp -f libamdgpu.so "$LIB_DIR/"
    sudo ldconfig -m "$LIB_DIR"
    
    cp -f rmapi_client_demo "$HOME/amd_rmapi_client_demo"
    chmod +x "$HOME/amd_rmapi_client_demo"
    
    # Install test suite
    sudo cp -f tests/test_suite "$INSTALL_DIR/amd_test_suite"
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
echo "✅ BUILD COMPLETE - FREEBSD"
echo "════════════════════════════════════════════════════════════════"
echo ""
echo "📊 Build Summary:"
echo "  • Driver binary:      rmapi_server ✅"
echo "  • Shared library:     libamdgpu.so ✅"
echo "  • Test suite:         tests/test_suite ✅"
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
    echo "  • Brain:             ./rmapi_server"
    echo "  • Library:           ./libamdgpu.so"
    echo "  • Test Suite:        ./tests/test_suite"
    echo "  • Client Demo:       ./rmapi_client_demo"
    echo ""
    echo "🛠️  Quick Start (Local):"
    echo "  1. Start server:     ./rmapi_server &"
    echo "  2. Run client:       ./rmapi_client_demo"
    echo "  3. Run tests:        ./tests/test_suite"
fi

echo ""
echo "📚 Documentation:"
echo "  • README:             ./README.md"
echo "  • Architecture:       ./docs/ARCHITECTURE_STEP_BY_STEP.md"
echo "  • Master Guide:       ../MAESTRO.md"
echo "  • Tests Guide:        ./tests/README.md"
echo ""
echo "════════════════════════════════════════════════════════════════"
echo "🎉 SUCCESS - FreeBSD Ready - HIT Edition"
echo "════════════════════════════════════════════════════════════════"

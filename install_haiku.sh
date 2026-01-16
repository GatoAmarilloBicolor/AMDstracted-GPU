#!/bin/bash

# 🏁 HIT Edition: Haiku Installer
# Builds and installs the AMD driver for Haiku OS
# Includes: driver, shared library, tests, and Accelerant
# Developed by: Haiku Imposible Team (HIT)

set -e

echo "════════════════════════════════════════════════════════════════"
echo "🚀 HIT Haiku Installation - Complete Build"
echo "════════════════════════════════════════════════════════════════"
echo ""

# Check prerequisites
echo "📋 Checking prerequisites..."
if ! command -v gcc &> /dev/null; then
    echo "❌ GCC not found. Install Haiku development tools."
    exit 1
fi
if ! command -v make &> /dev/null; then
    echo "❌ Make not found."
    exit 1
fi
echo "✅ Prerequisites OK"
echo ""

# 1. Build main driver for Haiku
echo "────────────────────────────────────────────────────────────────"
echo "📦 Step 1: Building Main Driver for Haiku"
echo "────────────────────────────────────────────────────────────────"
OS=haiku USERLAND_MODE=1 make clean all
if [ $? -ne 0 ]; then
    echo "❌ Driver build failed!"
    exit 1
fi
echo "✅ Driver built successfully for Haiku"
echo ""

# 2. Build tests
echo "────────────────────────────────────────────────────────────────"
echo "🧪 Step 2: Building Test Suite"
echo "────────────────────────────────────────────────────────────────"
cd tests
make -f Makefile.test clean
make -f Makefile.test
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

# 4. Install to Haiku system paths
echo "────────────────────────────────────────────────────────────────"
echo "📂 Step 4: Haiku Installation"
echo "────────────────────────────────────────────────────────────────"

# Haiku non-packaged directory
HAIKU_COMMON=/boot/home/config/non-packaged
INSTALL_DIR="$HAIKU_COMMON/bin"
LIB_DIR="$HAIKU_COMMON/lib"
ADDONS_DIR="$HAIKU_COMMON/add-ons/accelerants"

mkdir -p "$INSTALL_DIR"
mkdir -p "$LIB_DIR"
mkdir -p "$ADDONS_DIR"

echo "Installing to Haiku paths..."

# Install main artifacts
cp -f rmapi_server "$INSTALL_DIR/amd_rmapi_server"
chmod +x "$INSTALL_DIR/amd_rmapi_server"

cp -f libamdgpu.so "$LIB_DIR/"

cp -f rmapi_client_demo "$INSTALL_DIR/amd_rmapi_client_demo"
chmod +x "$INSTALL_DIR/amd_rmapi_client_demo"

# Install test suite
cp -f tests/test_suite "$INSTALL_DIR/amd_test_suite"
chmod +x "$INSTALL_DIR/amd_test_suite"

# Install Accelerant if built
if [ -f "amdgpu_hit.accelerant" ]; then
    cp -f amdgpu_hit.accelerant "$ADDONS_DIR/"
    echo "✅ Accelerant installed"
fi

echo "✅ Haiku installation complete"
echo ""

# 5. Summary
echo "════════════════════════════════════════════════════════════════"
echo "✅ BUILD COMPLETE - HAIKU"
echo "════════════════════════════════════════════════════════════════"
echo ""
echo "📊 Build Summary:"
echo "  • Driver binary:      rmapi_server ✅"
echo "  • Shared library:     libamdgpu.so ✅"
echo "  • Test suite:         tests/test_suite ✅"
echo "  • Client demo:        rmapi_client_demo ✅"
echo ""
echo "📍 Installation Paths:"
echo "  • Brain:             $INSTALL_DIR/amd_rmapi_server"
echo "  • Library:           $LIB_DIR/libamdgpu.so"
echo "  • Test Suite:        $INSTALL_DIR/amd_test_suite"
echo "  • Client Demo:       $INSTALL_DIR/amd_rmapi_client_demo"
if [ -f "amdgpu_hit.accelerant" ]; then
    echo "  • Accelerant:        $ADDONS_DIR/amdgpu_hit.accelerant"
fi
echo ""
echo "🛠️  Quick Start:"
echo "  1. Start server:     $INSTALL_DIR/amd_rmapi_server &"
echo "  2. Run client:       $INSTALL_DIR/amd_rmapi_client_demo"
echo "  3. Run tests:        $INSTALL_DIR/amd_test_suite"
echo ""
echo "════════════════════════════════════════════════════════════════"
echo "🎉 SUCCESS - Haiku Ready - HIT Edition"
echo "════════════════════════════════════════════════════════════════"

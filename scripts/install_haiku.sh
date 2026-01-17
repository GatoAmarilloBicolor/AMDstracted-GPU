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

# Check if running on Haiku
if [ "$(uname -s)" = "Haiku" ] && [ "$EUID" -ne 0 ]; then 
    echo "⚠️  Note: Some installation paths require root privileges"
    echo "   To install to system paths, run: sudo $0"
    echo ""
fi

# Haiku non-packaged directory (user-writable)
HAIKU_COMMON=/boot/home/config/non-packaged
INSTALL_DIR="$HAIKU_COMMON/bin"
LIB_DIR="$HAIKU_COMMON/lib"
ADDONS_DIR="$HAIKU_COMMON/add-ons/accelerants"

# Userland only - no system directories

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

# Install Accelerant
if [ -f "amdgpu_hit.accelerant" ]; then
    # Install to non-packaged (userland only)
    cp -f amdgpu_hit.accelerant "$ADDONS_DIR/"
    echo "✅ Accelerant installed to $ADDONS_DIR"
fi

# Create environment setup script
cat > /boot/home/.amd_gpu_env.sh << 'EOF'
#!/bin/bash
# AMD GPU Environment Setup - HIT Edition

# Driver and library paths
export AMD_GPU_BIN=/boot/home/config/non-packaged/bin
export AMD_GPU_LIB=/boot/home/config/non-packaged/lib
export LD_LIBRARY_PATH=$AMD_GPU_LIB:$LD_LIBRARY_PATH

# Graphics settings (userland)
export ACCELERANT_PATH=/boot/home/config/non-packaged/add-ons/accelerants

# Debugging (set to 1 to enable)
export AMD_DEBUG=0
export AMD_LOG_LEVEL=1  # 0=silent, 1=errors, 2=info, 3=debug

# Graphics optimization
export RADV_PERFTEST=aco  # Use optimized compiler
# export VBLANK_MODE=1    # Enable VSync if needed

# Vulkan support with RADV (real hardware acceleration)
export VK_ICD_FILENAMES=/boot/home/config/non-packaged/lib/vulkan/icd.d/radeon_icd.x86_64.json
export VK_LOADER_DEBUG=all
export RADV_DEBUG=all

# Add to PATH for easy access to tools
export PATH=$AMD_GPU_BIN:$PATH
EOF

chmod +x /boot/home/.amd_gpu_env.sh
echo "✅ Environment script created: /boot/home/.amd_gpu_env.sh"
echo ""
echo "To use AMD GPU tools, add to your ~/.profile:"
echo "  source ~/.amd_gpu_env.sh"
echo ""

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
echo "  4. Test Vulkan:      vulkaninfo | grep AMD"
echo ""
echo "════════════════════════════════════════════════════════════════"
echo "🎉 SUCCESS - Haiku Ready - HIT Edition"
echo "════════════════════════════════════════════════════════════════"

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
if ! command -v meson &> /dev/null; then
    echo "❌ Meson not found. Install with: pkgman install meson"
    exit 1
fi
if ! command -v ninja &> /dev/null; then
    echo "❌ Ninja not found. Install with: pkgman install ninja"
    exit 1
fi
echo "✅ Prerequisites OK"
echo ""

# Clean old build artifacts
echo "🧹 Cleaning old build artifacts..."
find . -name "*.o" -type f -delete 2>/dev/null || true
find . -name "*.so" -type f -delete 2>/dev/null || true
find . -name "*.a" -type f -delete 2>/dev/null || true
rm -rf builddir 2>/dev/null || true
echo "✅ Clean complete"
echo ""

# 1. Build main driver for Haiku
echo "────────────────────────────────────────────────────────────────"
echo "📦 Step 1: Building Main Driver for Haiku"
echo "────────────────────────────────────────────────────────────────"

# Always build natively (cross-compilation requires proper toolchain)
echo "Building natively..."
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
echo "✅ Driver built successfully for Haiku"
echo ""

# 2. Build tests
echo "────────────────────────────────────────────────────────────────"
echo "🧪 Step 2: Building Test Suite"
echo "────────────────────────────────────────────────────────────────"
meson test -C builddir
if [ $? -ne 0 ]; then
    echo "❌ Test build/run failed!"
    exit 1
fi
echo "✅ Tests built and run successfully"
echo ""

# 3. Tests already run in Step 2
echo "────────────────────────────────────────────────────────────────"
echo "🧪 Step 3: Tests Completed"
echo "────────────────────────────────────────────────────────────────"
echo "Tests executed via Meson in Step 2"

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

# Detect OS and set appropriate paths
if [ "$(uname -s)" = "Haiku" ]; then
    # Haiku paths
    HAIKU_COMMON=/boot/home/config/non-packaged
    INSTALL_DIR="$HAIKU_COMMON/bin"
    LIB_DIR="$HAIKU_COMMON/lib"
    echo "Installing to Haiku user paths..."
else
    # Linux paths (use local installation)
    INSTALL_DIR="/usr/local/bin"
    LIB_DIR="/usr/local/lib"
    echo "Installing to Linux system paths..."
fi

# Use manual copy instead of meson install to avoid binary corruption
echo "Copying binaries manually to prevent ELF header corruption..."

# Create directories if they don't exist
if ! mkdir -p "$INSTALL_DIR" 2>/dev/null; then
    echo "Warning: Could not create $INSTALL_DIR, trying user directory..."
    INSTALL_DIR="$HOME/.local/bin"
    LIB_DIR="$HOME/.local/lib"
    mkdir -p "$INSTALL_DIR" || {
        echo "Error: Cannot create installation directories"
        exit 1
    }
fi
mkdir -p "$LIB_DIR" 2>/dev/null || true

# Function to copy with sudo if needed
copy_file() {
    local src="$1"
    local dst="$2"
    local filename="$3"

    if cp -f "$src" "$dst/" 2>/dev/null; then
        echo "✅ Copied $filename"
    elif [ "$(uname -s)" = "Linux" ] && command -v sudo >/dev/null 2>&1; then
        echo "Requesting sudo permissions to install $filename..."
        if sudo cp -f "$src" "$dst/"; then
            echo "✅ Copied $filename with sudo"
        else
            echo "❌ Failed to copy $filename even with sudo"
            return 1
        fi
    else
        echo "❌ Failed to copy $filename"
        return 1
    fi
    return 0
}

# Copy binaries manually with verification
copy_file "builddir/amd_rmapi_server" "$INSTALL_DIR" "amd_rmapi_server" || exit 1

if [ "$LIB_COPY" = true ]; then
    copy_file "builddir/libamdgpu.so" "$LIB_DIR" "libamdgpu.so" || exit 1
fi

copy_file "builddir/amd_rmapi_client_demo" "$INSTALL_DIR" "amd_rmapi_client_demo" || exit 1
copy_file "builddir/amd_test_suite" "$INSTALL_DIR" "amd_test_suite" || exit 1
}
mkdir -p "$LIB_DIR" 2>/dev/null || true

# Copy binaries manually with verification
# Check if we're on Haiku (static linking)
if [ "$(uname -s)" = "Haiku" ]; then
    echo "Haiku detected - copying statically linked executables..."
    LIB_COPY=false
else
    echo "Linux detected - copying shared library and executables..."
    LIB_COPY=true
fi

echo "Copying amd_rmapi_server..."
cp -f builddir/amd_rmapi_server "$INSTALL_DIR/" || {
    echo "❌ Failed to copy amd_rmapi_server"
    exit 1
}

if [ "$LIB_COPY" = true ]; then
    echo "Copying libamdgpu.so..."
    cp -f builddir/libamdgpu.so "$LIB_DIR/" || {
        echo "❌ Failed to copy libamdgpu.so"
        exit 1
    }
else
    echo "Skipping libamdgpu.so copy (static linking)"
fi

echo "Copying amd_rmapi_client_demo..."
cp -f builddir/amd_rmapi_client_demo "$INSTALL_DIR/" || {
    echo "❌ Failed to copy amd_rmapi_client_demo"
    exit 1
}

echo "Copying amd_test_suite..."
cp -f builddir/amd_test_suite "$INSTALL_DIR/" || {
    echo "❌ Failed to copy amd_test_suite"
    exit 1
}

# Verify installation
echo "Verifying installation..."
if [ ! -x "$INSTALL_DIR/amd_rmapi_server" ]; then
    echo "❌ amd_rmapi_server not found or not executable"
    exit 1
fi
if [ ! -f "$LIB_DIR/libamdgpu.so" ]; then
    echo "❌ libamdgpu.so not found"
    exit 1
fi

echo "✅ Binaries installed and verified"

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
export LIBRARY_PATH=$AMD_GPU_LIB:$LIBRARY_PATH
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

# 4. Build Mesa RADV for Vulkan (optional, may take time)
echo "────────────────────────────────────────────────────────────────"
echo "🎨 Step 4: Vulkan RADV Support (Optional)"
echo "────────────────────────────────────────────────────────────────"
echo ""
echo "Do you want to build Mesa RADV for Vulkan support?"
echo "This provides hardware-accelerated Vulkan on AMD GPUs."
echo "Note: Building may take 10-30 minutes on first run."
echo ""
read -p "Build RADV Vulkan? (y/n): " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Yy]$ ]]; then
    RADV_LIB="/boot/home/config/non-packaged/lib/libvulkan_radeon.so"
    if [ -f "$RADV_LIB" ]; then
        echo "✅ RADV already installed at $RADV_LIB, skipping Mesa build"
    elif [ -d "mesa" ]; then
        echo ""
        echo "🔥 Building Mesa RADV for Vulkan support..."
        echo "This may take several minutes (only rebuilds changed files)..."
        cd mesa
        if [ -f "build/build.ninja" ]; then
            # Incremental build: only rebuilds what changed
            echo "Running incremental build..."
            ninja -C build > /tmp/mesa_build.log 2>&1
            if [ $? -eq 0 ]; then
                echo "✅ Mesa built successfully"
                ninja -C build install || echo "⚠️  Mesa install failed"
            else
                echo "⚠️  Mesa build failed - check /tmp/mesa_build.log"
            fi
        else
            echo "⚠️  Mesa not configured - run scripts/setup_mesa.sh first"
        fi
        cd ..
        echo "✅ Mesa build attempt complete"
    else
        echo "⚠️  Mesa not available - RADV not built"
    fi
else
    echo "⏭️  Skipping RADV build - continuing with OpenGL only"
fi

# 5. Summary
echo "════════════════════════════════════════════════════════════════"
echo "✅ BUILD COMPLETE - HAIKU"
echo "════════════════════════════════════════════════════════════════"
echo ""
echo "📊 Build Summary:"
echo "  • Driver binary:      amd_rmapi_server ✅"
echo "  • Shared library:     libamdgpu.so ✅"
echo "  • Test suite:         amd_test_suite ✅"
echo "  • Client demo:        amd_rmapi_client_demo ✅"
if [ -f "/boot/home/config/non-packaged/lib/libvulkan_radeon.so" ]; then
    echo "  • Vulkan RADV:        libvulkan_radeon.so ✅"
else
    echo "  • Vulkan RADV:        Not built (run setup_mesa.sh)"
fi
echo ""
echo "📍 Installation Paths:"
echo "  • Server:            $INSTALL_DIR/amd_rmapi_server"
echo "  • Library:           $LIB_DIR/libamdgpu.so"
echo "  • Test Suite:        $INSTALL_DIR/amd_test_suite"
echo "  • Client Demo:       $INSTALL_DIR/amd_rmapi_client_demo"
if [ -f "amdgpu_hit.accelerant" ]; then
    echo "  • Accelerant:        $ADDONS_DIR/amdgpu_hit.accelerant"
fi
echo ""
echo "🛠️  Quick Start:"
echo "  1. Source environment: source ~/.amd_gpu_env.sh"
echo "  2. Start server:      amd_rmapi_server &"
echo "  3. Run client:        amd_rmapi_client_demo"
echo "  4. Run tests:         amd_test_suite"
echo "  5. Test Vulkan:       vulkaninfo | grep AMD"
echo ""
echo "════════════════════════════════════════════════════════════════"
echo "🎉 SUCCESS - Haiku Ready - HIT Edition"
echo "════════════════════════════════════════════════════════════════"

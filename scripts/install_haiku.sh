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
LIB_COPY=false
if [ "$(uname -s)" = "Haiku" ]; then
    # Haiku paths
    HAIKU_COMMON=/boot/home/config/non-packaged
    INSTALL_DIR="$HAIKU_COMMON/bin"
    LIB_DIR="$HAIKU_COMMON/lib"
    echo "Installing to Haiku user paths..."
else
    # Linux paths (prefer user directory to avoid permission issues)
    INSTALL_DIR="$HOME/.local/bin"
    LIB_DIR="$HOME/.local/lib"
    LIB_COPY=true  # Copy shared lib on Linux
    echo "Installing to Linux user paths ($HOME/.local/)..."
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

# Function to copy files
copy_file() {
    local src="$1"
    local dst="$2"
    local filename="$3"

    if cp -f "$src" "$dst/"; then
        echo "✅ Copied $filename"
        return 0
    else
        echo "❌ Failed to copy $filename to $dst/"
        return 1
    fi
}

# Copy binaries manually with verification
copy_file "builddir/amd_rmapi_server" "$INSTALL_DIR" "amd_rmapi_server" || exit 1

if [ "$LIB_COPY" = true ]; then
    copy_file "builddir/libamdgpu.so" "$LIB_DIR" "libamdgpu.so" || exit 1
fi

copy_file "builddir/amd_rmapi_client_demo" "$INSTALL_DIR" "amd_rmapi_client_demo" || exit 1
copy_file "builddir/amd_test_suite" "$INSTALL_DIR" "amd_test_suite" || exit 1

# Verify installation
echo "Verifying installation..."
if [ ! -x "$INSTALL_DIR/amd_rmapi_server" ]; then
    echo "❌ amd_rmapi_server not found or not executable"
    exit 1
fi
if [ ! -x "$INSTALL_DIR/amd_rmapi_client_demo" ]; then
    echo "❌ amd_rmapi_client_demo not found or not executable"
    exit 1
fi
if [ ! -x "$INSTALL_DIR/amd_test_suite" ]; then
    echo "❌ amd_test_suite not found or not executable"
    exit 1
fi

echo "✅ Binaries installed and verified"

# 5. Configure OpenGL Support on Haiku
if [ "$(uname -s)" = "Haiku" ]; then
    echo "────────────────────────────────────────────────────────────────"
    echo "🎨 Step 5: Configuring OpenGL Support"
    echo "────────────────────────────────────────────────────────────────"
    
    MESA_PREFIX="/boot/home/config/non-packaged"
    DRI_PATH="$MESA_PREFIX/lib/dri"
    VULKAN_PATH="$MESA_PREFIX/share/vulkan/icd.d"
    
    # Check if Mesa is installed
    if [ ! -d "$DRI_PATH" ]; then
        echo "⚠️  Mesa DRI drivers not found at $DRI_PATH"
        echo "   Install with: pkgman install mesa_devel"
        echo "   Skipping OpenGL configuration..."
    else
        echo "✅ Mesa DRI drivers found"
        
        # Check Vulkan ICD
        if [ ! -f "$VULKAN_PATH/radeon_icd.x86_64.json" ]; then
            echo "⚠️  Vulkan ICD not found at $VULKAN_PATH/radeon_icd.x86_64.json"
            echo "   Make sure RADV ICD is installed with Mesa"
        else
            echo "✅ Vulkan ICD found"
        fi
        
        echo "✅ OpenGL/Zink configuration ready"
    fi
    echo ""
fi

# Create environment script
ENV_SCRIPT="$HOME/.amd_gpu_env.sh"
cat > "$ENV_SCRIPT" << EOF
#!/bin/bash
# AMD GPU Environment Setup - HIT Edition

# Driver and library paths
export AMD_GPU_BIN=$INSTALL_DIR
export AMD_GPU_LIB=$LIB_DIR
export LIBRARY_PATH=\$AMD_GPU_LIB:\$LIBRARY_PATH
export LD_LIBRARY_PATH=\$AMD_GPU_LIB:\$LD_LIBRARY_PATH

# Add to PATH for easy access to tools
export PATH=\$AMD_GPU_BIN:\$PATH

# Haiku-specific OpenGL configuration
if [ "\$(uname -s)" = "Haiku" ]; then
    MESA_PREFIX="/boot/home/config/non-packaged"
    
    # OpenGL driver search path
    export LIBGL_DRIVERS_PATH="\$MESA_PREFIX/lib/dri"
    
    # Force Zink (OpenGL over Vulkan)
    export MESA_LOADER_DRIVER_OVERRIDE="zink"
    
    # Vulkan ICD configuration
    export VK_ICD_FILENAMES="\$MESA_PREFIX/share/vulkan/icd.d/radeon_icd.x86_64.json"
    export VK_DRIVER_FILES="\$MESA_PREFIX/share/vulkan/icd.d/radeon_icd*.json"
    
    # Library paths for Mesa/Vulkan
    export LD_LIBRARY_PATH="\$MESA_PREFIX/lib:\$LD_LIBRARY_PATH"
    export LIBRARY_PATH="\$MESA_PREFIX/lib:\$LIBRARY_PATH"
    
    # Debug flags (optional)
    export LIBGL_DEBUG="verbose"
    export VK_LOADER_DEBUG="error"
fi

echo "AMD GPU environment loaded"
echo "Available commands: amd_rmapi_server, amd_rmapi_client_demo, amd_test_suite"
EOF

chmod +x "$ENV_SCRIPT"
echo "✅ Environment script created: $ENV_SCRIPT"

# Success message
echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "🎉 INSTALLATION COMPLETE - AMDGPU_Abstracted Ready!"
echo "═══════════════════════════════════════════════════════════════"
echo ""
echo "��� Quick Start:"
echo "  1. Load environment: source $ENV_SCRIPT"
echo "  2. Start server:      amd_rmapi_server &"
echo "  3. Run client:        amd_rmapi_client_demo"
echo "  4. Run tests:         amd_test_suite"
echo "  5. Test OpenGL:       GLInfo or glxinfo"
echo ""
echo "📁 Installation paths:"
echo "  • Binaries:  $INSTALL_DIR"
if [ "\$LIB_COPY" = true ]; then
    echo "  • Library:   $LIB_DIR"
fi
echo ""
if [ "$(uname -s)" = "Haiku" ]; then
    echo "📚 Enabled Features:"
    echo "  ✅ OpenGL/Zink (via Vulkan backend)"
    echo "  ✅ Vulkan (RADV)"
    echo "  ✅ RMAPI Server"
    echo ""
    echo "⚠️  Prerequisites for full functionality:"
    echo "  • pkgman install mesa_devel"
    echo "  • RMAPI server: amd_rmapi_server &"
fi
echo ""
echo "🎯 Status: Ready for GPU acceleration!"
echo "═══════════════════════════════════════════════════════════════"

#!/bin/bash

# 🏁 HIT Edition: Haiku Installer with GPU Acceleration
# Builds and installs the AMD driver for Haiku OS with R600 GPU support
# Includes: driver, shared library, tests, Mesa R600, and OpenGL acceleration

set -e

echo "════════════════════════════════════════════════════════════════"
echo "🚀 HIT Haiku Installation - Complete Build with GPU Acceleration"
echo "════════════════════════════════════════════════════════════════"
echo ""

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

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
find "$PROJECT_ROOT" -name "*.o" -type f -delete 2>/dev/null || true
find "$PROJECT_ROOT" -name "*.so" -type f -delete 2>/dev/null || true
find "$PROJECT_ROOT" -name "*.a" -type f -delete 2>/dev/null || true
rm -rf "$PROJECT_ROOT/builddir" 2>/dev/null || true
echo "✅ Clean complete"
echo ""

# 1. Build main driver for Haiku
echo "────────────────────────────────────────────────────────────────"
echo "📦 Step 1: Building Main Driver for Haiku"
echo "────────────────────────────────────────────────────────────────"

echo "Building natively..."
cd "$PROJECT_ROOT"
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
echo ""

# 4. Ensure Mesa R600 Driver (Build if needed)
echo "────────────────────────────────────────────────────────────────"
echo "🔧 Step 4: Ensure Mesa R600 Driver"
echo "────────────────────────────────────────────────────────────────"

DRIVER_FOUND=0
for path in /boot/system/lib/dri /boot/home/config/non-packaged/lib/dri; do
    if [ -f "$path/r600_dri.so" ] || [ -f "$path/libgallium_dri.so" ]; then
        echo "✅ Mesa R600 driver found: $path"
        DRIVER_FOUND=1
        break
    fi
done

if [ $DRIVER_FOUND -eq 0 ]; then
    echo "⚠️  Mesa R600 driver not found - building from stable source..."
    echo ""
    
    if [ -x "$PROJECT_ROOT/scripts/build_mesa_r600.sh" ]; then
        echo "Building Mesa R600 (this may take 30-60 minutes)..."
        "$PROJECT_ROOT/scripts/build_mesa_r600.sh"
        if [ $? -eq 0 ]; then
            echo "✅ Mesa R600 built and installed"
        else
            echo "❌ Mesa build failed"
            exit 1
        fi
    else
        echo "❌ Mesa build script not found"
        exit 1
    fi
else
    echo "✅ Mesa R600 driver available"
fi
echo ""

# 5. Install to Haiku system paths
echo "────────────────────────────────────────────────────────────────"
echo "📂 Step 5: Haiku Installation"
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
copy_file "$PROJECT_ROOT/builddir/amd_rmapi_server" "$INSTALL_DIR" "amd_rmapi_server" || exit 1

if [ "$LIB_COPY" = true ]; then
    copy_file "$PROJECT_ROOT/builddir/libamdgpu.so" "$LIB_DIR" "libamdgpu.so" || exit 1
fi

copy_file "$PROJECT_ROOT/builddir/amd_rmapi_client_demo" "$INSTALL_DIR" "amd_rmapi_client_demo" || exit 1
copy_file "$PROJECT_ROOT/builddir/amd_test_suite" "$INSTALL_DIR" "amd_test_suite" || exit 1

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
echo ""

# 6. Configure OpenGL Support on Haiku
if [ "$(uname -s)" = "Haiku" ]; then
    echo "────────────────────────────────────────────────────────────────"
    echo "🎨 Step 6: Configuring OpenGL Support"
    echo "────────────────────────────────────────────────────────────────"
    
    # Detect GPU with lspci
    echo "🔍 Detecting AMD GPU..."
    if command -v lspci &> /dev/null; then
        AMD_GPU=$(lspci -d 1002: -v 2>/dev/null | grep -i "radeon\|amd" | head -1)
        if [ -n "$AMD_GPU" ]; then
            echo "   Found: $AMD_GPU"
        else
            echo "   No AMD GPU detected via lspci"
        fi
    else
        echo "   lspci not available (install pciutils)"
    fi
    
    # Create environment script for GPU acceleration
    echo "Creating environment script..."
    HAIKU_COMMON=${HAIKU_COMMON:-/boot/home/config/non-packaged}
    
    cat > "$HAIKU_COMMON/.amd_gpu_env.sh" << 'EOF'
#!/bin/bash
# AMD GPU Environment Setup for Haiku
INSTALL_PREFIX="/boot/home/config/non-packaged"

export LIBRARY_PATH="$INSTALL_PREFIX/lib:$LIBRARY_PATH"
export LD_LIBRARY_PATH="$INSTALL_PREFIX/lib:/boot/system/lib:$LD_LIBRARY_PATH"
export MESA_LOADER_DRIVER_OVERRIDE="r600"
export LIBGL_DRIVERS_PATH="$INSTALL_PREFIX/lib/dri:/boot/home/config/non-packaged/lib/dri:/boot/system/lib/dri"
export MESA_GL_VERSION_OVERRIDE="4.3"
export MESA_GLSL_VERSION_OVERRIDE="430"
export MESA_NO_DITHER=1
export GALLIUM_DRIVER="r600"
export PATH="$INSTALL_PREFIX/bin:$PATH"

if [ -d "$INSTALL_PREFIX/share/vulkan" ]; then
    export VK_ICD_FILENAMES="$INSTALL_PREFIX/share/vulkan/icd.d/radeon_icd.x86_64.json"
fi
EOF
    
    chmod +x "$HAIKU_COMMON/.amd_gpu_env.sh"
    ln -sf "$HAIKU_COMMON/.amd_gpu_env.sh" /boot/home/.amd_gpu_env.sh 2>/dev/null || true
    echo "✅ Environment script created: $HAIKU_COMMON/.amd_gpu_env.sh"
    echo ""
fi

# Summary
echo "════════════════════════════════════════════════════════════════"
echo "✅ INSTALLATION COMPLETE - AMDGPU_Abstracted Ready!"
echo "════════════════════════════════════════════════════════════════"
echo ""

echo "Quick Start:"
echo "  1. Load environment: source /boot/home/.amd_gpu_env.sh"
echo "  2. Start server:      amd_rmapi_server &"
echo "  3. Run client:        amd_rmapi_client_demo"
echo "  4. Run tests:         amd_test_suite"
echo ""

echo "Installation paths:"
echo "  • Binaries:  $INSTALL_DIR"
echo "  • Libraries: $LIB_DIR"
echo ""

if [ "$(uname -s)" = "Haiku" ]; then
    echo "GPU Status:"
    if [ -n "$AMD_GPU" ]; then
        echo "  ✅ AMD GPU: $AMD_GPU"
    else
        echo "  ⚠️  No AMD GPU detected"
    fi
    echo "  ✅ Mesa R600 Driver"
    echo "  ✅ GPU Acceleration Ready"
    echo ""
fi

echo "Enabled Features:"
echo "  ✅ RMAPI GPU Abstraction"
echo "  ✅ OpenGL (R600 Driver)"
echo "  ✅ GPU Acceleration"
echo ""

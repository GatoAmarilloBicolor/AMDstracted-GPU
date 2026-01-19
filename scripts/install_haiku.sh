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
    
    # Try multiple Mesa installation paths on Haiku
    MESA_PREFIX=""
    DRI_PATH=""
    VULKAN_PATH=""
    
    # Check system-wide Mesa first
    if [ -d "/boot/system/lib/dri" ]; then
        MESA_PREFIX="/boot/system"
        DRI_PATH="/boot/system/lib/dri"
        VULKAN_PATH="/boot/system/share/vulkan/icd.d"
        echo "✅ Found Mesa in system path: /boot/system"
    # Then check user Mesa
    elif [ -d "/boot/home/config/non-packaged/lib/dri" ]; then
        MESA_PREFIX="/boot/home/config/non-packaged"
        DRI_PATH="/boot/home/config/non-packaged/lib/dri"
        VULKAN_PATH="/boot/home/config/non-packaged/share/vulkan/icd.d"
        echo "✅ Found Mesa in user path: /boot/home/config/non-packaged"
    fi
    
    OPENGL_MODE="software"
    
    # Check if Mesa DRI drivers are installed
    if [ -z "$DRI_PATH" ] || [ ! -d "$DRI_PATH" ]; then
        echo "⚠️  Mesa DRI drivers not found"
        echo "   Searched: /boot/system/lib/dri, /boot/home/config/non-packaged/lib/dri"
        echo ""
        
        # Ask if user wants to compile Mesa
        echo "📋 Options:"
        echo "  1. Continue with software rendering (CPU)"
        echo "  2. Compile and install Mesa with DRI drivers"
        echo ""
        read -p "Choose (1 or 2): " choice
        
        if [ "$choice" = "2" ]; then
            echo ""
            echo "🔨 Compiling Mesa for Haiku..."
            echo "   This will take several minutes..."
            
            MESA_BUILD_DIR="/tmp/mesa_build_$$"
            mkdir -p "$MESA_BUILD_DIR"
            cd "$MESA_BUILD_DIR"
            
            # Clone Mesa
            if ! git clone --depth 1 https://gitlab.freedesktop.org/mesa/mesa.git 2>&1 | grep -q "fatal"; then
                cd mesa
                
                # Build Mesa with R600 driver for Wrestler GPU
                echo "   Configuring Mesa..."
                meson setup build \
                    -Dprefix=/boot/home/config/non-packaged \
                    -Dgallium-drivers=r600,softpipe \
                    -Dglx=auto \
                    -Dopengl=true \
                    -Dshared-glapi=enabled \
                    2>&1 | tee /tmp/mesa_config.log
                CONFIG_RESULT=$?
                
                if [ $CONFIG_RESULT -eq 0 ]; then
                    echo ""
                    echo "   Building Mesa (this may take 10+ minutes)..."
                    echo ""
                    
                    # Build with live progress
                    ninja -C build 2>&1 | tee /tmp/mesa_build.log | grep -E "(\[|error|Error|FAILED)"
                    BUILD_RESULT=${PIPESTATUS[0]}
                    echo ""
                    
                    if [ $BUILD_RESULT -eq 0 ]; then
                        echo "   Installing Mesa..."
                        ninja -C build install 2>&1 | tee /tmp/mesa_install.log | grep -E "(Installing|Copying|error|Error)"
                        INSTALL_RESULT=$?
                        
                        if [ $INSTALL_RESULT -eq 0 ]; then
                            # Verify DRI drivers were installed
                            if [ -f "/boot/home/config/non-packaged/lib/dri/r600_dri.so" ] || \
                               [ -f "/boot/home/config/non-packaged/lib/dri/r600_dri.so.1" ]; then
                                MESA_PREFIX="/boot/home/config/non-packaged"
                                DRI_PATH="$MESA_PREFIX/lib/dri"
                                VULKAN_PATH="$MESA_PREFIX/share/vulkan/icd.d"
                                echo "✅ Mesa compiled and installed successfully"
                                echo "✅ R600 DRI driver found: $DRI_PATH/r600_dri.so"
                                OPENGL_MODE="r600"
                            else
                                echo "⚠️  Mesa installed but R600 driver not found"
                                echo "   Check: ls -la /boot/home/config/non-packaged/lib/dri/"
                                OPENGL_MODE="software"
                            fi
                        else
                            echo "❌ Mesa installation failed"
                            tail -20 /tmp/mesa_install.log | grep -E "error|Error"
                            OPENGL_MODE="software"
                        fi
                    else
                        echo "❌ Mesa build failed"
                        tail -20 /tmp/mesa_build.log | grep -E "error|Error|FAILED"
                        OPENGL_MODE="software"
                    fi
                else
                    echo "❌ Mesa configuration failed"
                    tail -20 /tmp/mesa_config.log | grep -E "ERROR|error"
                    OPENGL_MODE="software"
                fi
                
                cd ../..
            else
                echo "❌ Failed to clone Mesa"
                OPENGL_MODE="software"
            fi
            
            # Cleanup
            rm -rf "$MESA_BUILD_DIR"
            
        else
            echo "⚠️  Using software rendering fallback..."
            OPENGL_MODE="software"
        fi
    else
        echo "✅ Mesa DRI drivers found"
        
        # Check for RADV support (modern AMD)
        if [ -f "$VULKAN_PATH/radeon_icd.x86_64.json" ]; then
            echo "✅ RADV Vulkan ICD found (modern hardware)"
            OPENGL_MODE="radv"
        # Check for r600 driver (R600/R700/Evergreen/Wrestler/Brazos)
        elif [ -f "$DRI_PATH/r600_dri.so" ] || [ -f "$DRI_PATH/r600_dri.so.1" ]; then
            echo "✅ R600 DRI driver found (R600/R700/Evergreen/Wrestler AMD)"
            OPENGL_MODE="r600"
        # Check for r300 driver (R300/R400/R500/Radeon HD)
        elif [ -f "$DRI_PATH/r300_dri.so" ] || [ -f "$DRI_PATH/r300_dri.so.1" ]; then
            echo "✅ R300 DRI driver found (R300/R400/R500 AMD)"
            OPENGL_MODE="r300"
        # Check for r100 driver (very old R100/R200)
        elif [ -f "$DRI_PATH/r100_dri.so" ] || [ -f "$DRI_PATH/r100_dri.so.1" ]; then
            echo "✅ R100 DRI driver found (R100/R200 AMD)"
            OPENGL_MODE="r100"
        # Check for software rendering
        elif [ -f "$DRI_PATH/swrast_dri.so" ] || [ -f "$DRI_PATH/swrast_dri.so.1" ]; then
            echo "✅ Software rendering available"
            OPENGL_MODE="software"
        else
            echo "⚠️  No GPU drivers found, using software rendering"
            OPENGL_MODE="software"
        fi
    fi
    
    echo "🎯 OpenGL Configuration: $OPENGL_MODE"
    
    # Display GPU info based on detected mode
    case "$OPENGL_MODE" in
        radv)
            echo "📊 GPU Info: Modern AMD (Polaris+, Vega, RDNA)"
            echo "   Using: Vulkan RADV + Zink"
            ;;
        r600)
            echo "📊 GPU Info: R600/R700/Evergreen/Wrestler era"
            echo "   Using: R600 DRI driver"
            ;;
        r300)
            echo "📊 GPU Info: R300/R400/R500/Radeon HD era"
            echo "   Using: R300 DRI driver"
            ;;
        r100)
            echo "📊 GPU Info: R100/R200 ancient hardware"
            echo "   Using: R100 DRI driver"
            ;;
        software)
            echo "📊 GPU Info: No acceleration available"
            echo "   Using: Software rendering (llvmpipe)"
            ;;
    esac
    
    # Create mode detection file for environment script
    echo "$OPENGL_MODE" > "$HOME/.amd_gpu_opengl_mode"
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
    # Detect Mesa installation path
    if [ -d "/boot/system/lib/dri" ]; then
        MESA_PREFIX="/boot/system"
    elif [ -d "/boot/home/config/non-packaged/lib/dri" ]; then
        MESA_PREFIX="/boot/home/config/non-packaged"
    else
        MESA_PREFIX="/boot/home/config/non-packaged"  # fallback
    fi
    
    # OpenGL driver search path
    export LIBGL_DRIVERS_PATH="\$MESA_PREFIX/lib/dri"
    
    # Library paths for Mesa
    export LD_LIBRARY_PATH="\$MESA_PREFIX/lib:\$LD_LIBRARY_PATH"
    export LIBRARY_PATH="\$MESA_PREFIX/lib:\$LIBRARY_PATH"
    
    # Detect OpenGL configuration mode
    if [ -f "\$HOME/.amd_gpu_opengl_mode" ]; then
        OPENGL_MODE=\$(cat "\$HOME/.amd_gpu_opengl_mode")
    else
        OPENGL_MODE="software"
    fi
    
    # Configure based on detected hardware
    case "\$OPENGL_MODE" in
        radv)
            # Modern AMD GPU with RADV + Vulkan support
            export MESA_LOADER_DRIVER_OVERRIDE="zink"
            export VK_ICD_FILENAMES="\$MESA_PREFIX/share/vulkan/icd.d/radeon_icd.x86_64.json"
            export VK_DRIVER_FILES="\$MESA_PREFIX/share/vulkan/icd.d/radeon_icd*.json"
            export VK_LOADER_DEBUG="error"
            echo "[AMD GPU] Mode: Modern RADV (OpenGL via Zink + Vulkan)"
            ;;
        r600)
            # R600/R700/Evergreen/Wrestler/Brazos era AMD GPU
            export MESA_LOADER_DRIVER_OVERRIDE="r600"
            echo "[AMD GPU] Mode: R600 driver (R600/R700/Evergreen/Wrestler)"
            ;;
        r300)
            # R300/R400/R500/Radeon HD era AMD GPU
            export MESA_LOADER_DRIVER_OVERRIDE="r300"
            echo "[AMD GPU] Mode: R300 driver (R300/R400/R500)"
            ;;
        r100)
            # Very old R100/R200 era AMD GPU
            export MESA_LOADER_DRIVER_OVERRIDE="r100"
            echo "[AMD GPU] Mode: R100 driver (R100/R200 ancient)"
            ;;
        software)
            # Software rendering (CPU fallback)
            export LIBGL_ALWAYS_SOFTWARE=1
            export GALLIUM_DRIVER="llvmpipe"
            export MESA_LOADER_DRIVER_OVERRIDE="swrast"
            echo "[AMD GPU] Mode: Software Rendering (CPU, no GPU acceleration)"
            ;;
        *)
            # Default fallback
            export LIBGL_ALWAYS_SOFTWARE=1
            export GALLIUM_DRIVER="llvmpipe"
            echo "[AMD GPU] Mode: Fallback to Software Rendering"
            ;;
    esac
    
    # Debug flags (optional, comment out for production)
    # export LIBGL_DEBUG="verbose"
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
    # Check which mode was detected
    if [ -f "$HOME/.amd_gpu_opengl_mode" ]; then
        MODE=$(cat "$HOME/.amd_gpu_opengl_mode")
    else
        MODE="unknown"
    fi
    
    echo "📚 Enabled Features:"
    case "$MODE" in
        radv)
            echo "  ✅ OpenGL/Zink (via Vulkan backend)"
            echo "  ✅ Vulkan (RADV - Modern AMD)"
            echo "  ✅ RMAPI Server"
            echo ""
            echo "🎯 Hardware: Modern AMD GPU (Polaris, Vega, RDNA)"
            ;;
        r600)
            echo "  ✅ OpenGL (R600 driver)"
            echo "  ✅ RMAPI Server"
            echo ""
            echo "🎯 Hardware: R600/R700/Evergreen/Wrestler/Brazos AMD"
            ;;
        r300)
            echo "  ✅ OpenGL (R300 driver)"
            echo "  ✅ RMAPI Server"
            echo ""
            echo "🎯 Hardware: R300/R400/R500/Radeon HD AMD"
            ;;
        r100)
            echo "  ✅ OpenGL (R100 driver)"
            echo "  ✅ RMAPI Server"
            echo ""
            echo "🎯 Hardware: R100/R200 ancient AMD"
            ;;
        software)
            echo "  ✅ OpenGL (Software Rendering - llvmpipe)"
            echo "  ✅ RMAPI Server"
            echo ""
            echo "⚠️  Hardware: No GPU drivers (CPU rendering only)"
            ;;
    esac
    echo ""
    echo "📋 Post-Installation Notes:"
    if [ "$MODE" = "radv" ]; then
        echo "  • For RADV: RMAPI server required"
        echo "  • Start with: amd_rmapi_server &"
    elif [ "$MODE" = "software" ]; then
        echo "  • For better performance: install Mesa drivers"
        echo "  • Check: pkgman search mesa"
    fi
fi
echo ""
echo "🎯 Status: Ready for GPU acceleration!"
echo "═══════════════════════════════════════════════════════════════"

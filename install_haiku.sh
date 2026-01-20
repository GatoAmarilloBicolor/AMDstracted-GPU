#!/bin/bash
# Install AMD Accelerant and Mesa on Haiku OS
# Sets up graphics acceleration for GPU support

set -e

baseDir="$PWD"

# Detect architecture
if command -v getarch &> /dev/null; then
    ARCH="$(getarch)"
    ON_HAIKU=true
else
    echo "[✗] This script must be run on Haiku OS"
    exit 1
fi

installDir="$baseDir/install.$ARCH"

echo "════════════════════════════════════════════════════════════"
echo "AMD Graphics Installation - Haiku OS"
echo "════════════════════════════════════════════════════════════"
echo ""
echo "Architecture: $ARCH"
echo "Install dir:  $installDir"
echo ""

# Verify installation directory exists
if [ ! -d "$installDir" ]; then
    echo "[✗] Installation directory not found: $installDir"
    echo ""
    echo "Please run the build scripts first:"
    echo "  ./Build.sh          (builds core + accelerant)"
    echo "  ./build_mesa.sh     (builds Mesa)"
    exit 1
fi

# Check what's available to install
ACCELERANT_AVAILABLE=false
MESA_AVAILABLE=false

if [ -f "$installDir/add-ons/accelerants/amd_gfx.accelerant" ]; then
    ACCELERANT_AVAILABLE=true
    echo "[✓] Accelerant found"
fi

if [ -f "$installDir/lib/libGL.so" ] || [ -f "$installDir/lib/libGL.so.1" ]; then
    MESA_AVAILABLE=true
    echo "[✓] Mesa found"
fi

echo ""

if [ "$ACCELERANT_AVAILABLE" = false ] && [ "$MESA_AVAILABLE" = false ]; then
    echo "[✗] No components found to install"
    echo ""
    echo "Build first:"
    echo "  ./Build.sh          # Core + Accelerant"
    echo "  ./build_mesa.sh     # Mesa (optional but recommended)"
    exit 1
fi

# Check if running as root (needed for system install)
if [ "$EUID" -ne 0 ]; then
    echo "[!] This script needs to run as root to install to system directories"
    echo ""
    echo "Please run with sudo:"
    echo "  sudo ./install_haiku.sh"
    echo ""
    echo "Or install to user directory (prefix with ~):"
    echo "  mkdir -p ~/.config/add-ons/accelerants"
    echo "  cp $installDir/add-ons/accelerants/* ~/.config/add-ons/accelerants/"
    exit 1
fi

echo ""
echo "════════════════════════════════════════════════════════════"
echo "Installing components..."
echo "════════════════════════════════════════════════════════════"
echo ""

# Install accelerant
if [ "$ACCELERANT_AVAILABLE" = true ]; then
    echo "[1/3] Installing AMD Accelerant..."
    mkdir -p /boot/system/add-ons/accelerants
    cp "$installDir/add-ons/accelerants/amd_gfx.accelerant" \
       /boot/system/add-ons/accelerants/
    echo "[✓] Accelerant installed"
else
    echo "[SKIP] Accelerant not available"
fi

echo ""

# Install Mesa libraries
if [ "$MESA_AVAILABLE" = true ]; then
    echo "[2/3] Installing Mesa libraries..."
    
    # Install libraries
    mkdir -p /boot/system/lib
    cp "$installDir/lib/"*.so* /boot/system/lib/ 2>/dev/null || true
    
    # Install development files (optional)
    mkdir -p /boot/system/develop/lib/pkgconfig
    cp "$installDir/develop/lib/pkgconfig/"*.pc /boot/system/develop/lib/pkgconfig/ 2>/dev/null || true
    
    # Install headers (optional)
    mkdir -p /boot/system/develop/headers
    cp -r "$installDir/develop/headers/"* /boot/system/develop/headers/ 2>/dev/null || true
    
    echo "[✓] Mesa installed"
else
    echo "[SKIP] Mesa not available"
fi

echo ""
echo "[3/3] Configuring environment..."

# Create environment setup script
ENV_SCRIPT="/boot/system/lib/environment.d/amd_gpu.sh"
mkdir -p "$(dirname $ENV_SCRIPT)"

cat > "$ENV_SCRIPT" << 'EOF'
#!/bin/bash
# AMD GPU Environment Configuration - Auto-loaded by Haiku

# Add AMD GPU libraries to library path
if [ -d "/boot/home/src/AMDstracted-GPU/install.x86_64/lib" ]; then
    export LD_LIBRARY_PATH="/boot/home/src/AMDstracted-GPU/install.x86_64/lib:$LD_LIBRARY_PATH"
fi

# Add pkg-config path for development
if [ -d "/boot/home/src/AMDstracted-GPU/install.x86_64/lib/pkgconfig" ]; then
    export PKG_CONFIG_PATH="/boot/home/src/AMDstracted-GPU/install.x86_64/lib/pkgconfig:$PKG_CONFIG_PATH"
fi
EOF

chmod 755 "$ENV_SCRIPT"
echo "[✓] Environment configured"

echo ""
echo "════════════════════════════════════════════════════════════"
echo "[✓] Installation Complete!"
echo "════════════════════════════════════════════════════════════"
echo ""
echo "Next steps:"
echo ""
echo "1. IMPORTANT: Restart graphics system"
echo "   Command: pkill -9 app_server"
echo "   (Haiku will automatically restart it)"
echo ""
echo "2. Test graphics:"
echo "   - Open a graphics application (browser, image viewer, etc.)"
echo "   - Check if it renders smoothly"
echo "   - Check GPU usage with available tools"
echo ""
echo "3. Verify installation:"
echo "   - Check accelerant: ls -l /boot/system/add-ons/accelerants/amd_gfx.accelerant"
echo "   - Check Mesa: ls -l /boot/system/lib/libGL.so*"
echo ""
echo "Environment variables are auto-loaded by Haiku"
echo ""
echo "Troubleshooting:"
echo "  - If graphics don't work: check build logs"
echo "  - If app_server won't start: reinstall accelerant"
echo "  - For detailed testing: see documentation in project"
echo ""
echo "════════════════════════════════════════════════════════════"

#!/bin/bash
# DISRUPTIVE SOLUTION: Direct Mesa build without Meson hell
# For Haiku OS - Compiles Mesa directly with all verbose output

set -e

if ! command -v getarch &> /dev/null; then
    echo "[✗] ERROR: This script requires Haiku OS"
    exit 1
fi

baseDir="$PWD"
driver="${1:-r600}"

echo "════════════════════════════════════════════════════════════"
echo "MESA BUILD - DIRECT METHOD (No Meson Hell)"
echo "════════════════════════════════════════════════════════════"
echo ""
echo "Base dir: $baseDir"
echo "Driver:  $driver"
echo ""

# Step 1: Ensure Mesa source exists
if [ ! -d "mesa_source" ]; then
    echo "[*] Downloading Mesa 21.1.9..."
    wget -q -O mesa-21.1.9.tar.gz https://github.com/mesa3d/mesa/archive/refs/tags/21.1.9.tar.gz || \
    curl -s -L -o mesa-21.1.9.tar.gz https://github.com/mesa3d/mesa/archive/refs/tags/21.1.9.tar.gz
    
    echo "[*] Extracting..."
    tar xzf mesa-21.1.9.tar.gz
    mv mesa-21.1.9 mesa_source
    rm mesa-21.1.9.tar.gz
    echo "[✓] Mesa downloaded"
fi

# Step 2: Show environment
echo ""
echo "[*] Environment Check:"
echo "    Meson: $(meson --version)"
echo "    Ninja: $(ninja --version)"
echo "    GCC: $(gcc --version | head -1)"
echo ""

# Step 3: Enter source directory for Meson
echo "[*] Entering mesa_source directory..."
cd "$baseDir/mesa_source"
echo "[✓] Current dir: $(pwd)"
echo ""

# Step 4: Create build directory
buildDir="$baseDir/builddir_mesa"
installDir="$baseDir/install.x86_64"

if [ -d "$buildDir" ]; then
    echo "[*] Cleaning old build..."
    rm -rf "$buildDir"
fi

mkdir -p "$buildDir"
mkdir -p "$installDir/lib/pkgconfig"
echo "[✓] Directories created"
echo ""

# Step 5: Run Meson with FULL output
echo "[*] Running Meson Setup..."
echo "    Command: meson setup"
echo "    Build dir: $buildDir"
echo "    Driver: $driver"
echo ""

meson setup "$buildDir" \
    -Dprefix="$installDir" \
    -Dbuildtype=release \
    -Doptimization=3 \
    -Dgallium-drivers=$driver \
    -Dplatforms=haiku \
    -Dopengl=true \
    -Dglx=disabled \
    -Degl=disabled \
    -Dgles2=enabled \
    -Dshader-cache=enabled \
    -Dvulkan-drivers= \
    -Dllvm=disabled 2>&1 | tee meson_setup.log

if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo ""
    echo "[✗] MESON SETUP FAILED"
    echo "See: meson_setup.log"
    echo ""
    echo "Try this manually:"
    echo "  cd $(pwd)"
    echo "  rm -rf $buildDir"
    echo "  meson setup $buildDir -Dprefix=$installDir -Dgallium-drivers=$driver -Dplatforms=haiku"
    exit 1
fi

echo ""
echo "[✓] Meson setup complete"
echo ""

# Step 6: Build with Ninja
echo "[*] Starting Ninja build (this may take 30+ minutes)..."
echo ""

ninja -C "$buildDir" 2>&1 | tee ninja_build.log

if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo ""
    echo "[✗] NINJA BUILD FAILED"
    echo "See: ninja_build.log"
    exit 1
fi

echo ""
echo "[✓] Build complete"
echo ""

# Step 7: Install
echo "[*] Running Ninja install..."
ninja -C "$buildDir" install 2>&1 | tee ninja_install.log

if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo ""
    echo "[✗] NINJA INSTALL FAILED"
    echo "See: ninja_install.log"
    exit 1
fi

echo ""
echo "════════════════════════════════════════════════════════════"
echo "[✓] MESA BUILD SUCCESSFUL!"
echo "════════════════════════════════════════════════════════════"
echo ""
echo "Install dir: $installDir"
echo ""
echo "Next steps:"
echo "  1. sudo $baseDir/setup.sh install"
echo "  2. pkill -9 app_server"
echo "  3. open /boot/system/apps/WebPositive"
echo ""
echo "Logs saved:"
echo "  - meson_setup.log"
echo "  - ninja_build.log"
echo "  - ninja_install.log"
echo ""

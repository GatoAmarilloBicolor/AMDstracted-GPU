#!/bin/bash
# Prepare Mesa for Haiku - Download and setup compatible version
# This creates a local copy that works with Haiku's build system

set -e

baseDir="$PWD"

echo "════════════════════════════════════════════════════════════"
echo "Mesa Preparation for Haiku"
echo "════════════════════════════════════════════════════════════"
echo ""

# Haiku-compatible Mesa version
# Using Mesa 21.1.x which is known to work well on Haiku
MESA_VERSION="21.1.9"
MESA_URL="https://github.com/mesa3d/mesa/archive/refs/tags/$MESA_VERSION.tar.gz"
MESA_DIR="mesa-$MESA_VERSION"

echo "[INFO] Mesa version: $MESA_VERSION"
echo "[INFO] Download URL: $MESA_URL"
echo ""

# Check if already exists
if [ -d "$baseDir/mesa_source" ]; then
    echo "[!] mesa_source already exists"
    echo ""
    echo "Options:"
    echo "  1. Continue (skip download)"
    echo "  2. Remove and re-download"
    echo ""
    read -p "Choice (1 or 2): " choice
    
    if [ "$choice" = "2" ]; then
        echo "[INFO] Removing old mesa_source..."
        rm -rf "$baseDir/mesa_source"
    else
        echo "[INFO] Using existing mesa_source"
        exit 0
    fi
fi

# Download
echo "[INFO] Downloading Mesa $MESA_VERSION..."
if command -v wget &> /dev/null; then
    wget -O "$MESA_DIR.tar.gz" "$MESA_URL"
elif command -v curl &> /dev/null; then
    curl -L -o "$MESA_DIR.tar.gz" "$MESA_URL"
else
    echo "[✗] wget or curl required"
    exit 1
fi

echo "[✓] Downloaded"
echo ""

# Extract
echo "[INFO] Extracting..."
tar xzf "$MESA_DIR.tar.gz"
mv "$MESA_DIR" "mesa_source"
rm "$MESA_DIR.tar.gz"

echo "[✓] Extracted to mesa_source"
echo ""

# Create build wrapper
echo "[INFO] Creating Haiku build wrapper..."

cat > "$baseDir/mesa_source/haiku_build.sh" << 'SCRIPT'
#!/bin/bash
# Haiku-specific Mesa build wrapper

set -e

DRIVER="${1:-r600}"
BUILDDIR="${2:-../builddir_mesa}"
INSTALLDIR="${3:-../install.x86_64}"

echo "[INFO] Building Mesa for Haiku"
echo "  Driver: $DRIVER"
echo "  Build: $BUILDDIR"
echo "  Install: $INSTALLDIR"
echo ""

# Ensure directories exist
mkdir -p "$BUILDDIR"
mkdir -p "$INSTALLDIR/lib/pkgconfig"

# Run meson setup from this directory
# Haiku Meson expects: meson setup builddir [options]
meson setup "$BUILDDIR" \
    -Dprefix="$INSTALLDIR" \
    -Dbuildtype=release \
    -Doptimization=3 \
    -Dgallium-drivers=$DRIVER \
    -Dplatforms=haiku \
    -Dopengl=true \
    -Dglx=disabled \
    -Degl=disabled \
    -Dgles2=enabled \
    -Dshader-cache=enabled \
    -Dvulkan-drivers= \
    -Dllvm=disabled

echo ""
echo "[✓] Meson setup complete"
echo ""
echo "Next: ninja -C $BUILDDIR"
echo "Then:  ninja -C $BUILDDIR install"
SCRIPT

chmod +x "$baseDir/mesa_source/haiku_build.sh"

echo "[✓] Build wrapper created"
echo ""

echo "════════════════════════════════════════════════════════════"
echo "[✓] Mesa Ready!"
echo "════════════════════════════════════════════════════════════"
echo ""
echo "Build Mesa on Haiku:"
echo "  cd mesa_source"
echo "  ./haiku_build.sh r600  # For legacy Radeon"
echo "  ../ninja -C ../builddir_mesa"
echo "  ../ninja -C ../builddir_mesa install"
echo ""
echo "Or use: ./build_mesa_haiku_direct.sh"
echo ""

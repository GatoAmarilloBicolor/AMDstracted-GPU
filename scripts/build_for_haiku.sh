#!/bin/bash

# Build unified AMD driver for Haiku
# This script prepares and compiles for the Haiku platform

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
HAIKU_TOOLCHAIN="${HAIKU_TOOLCHAIN:-haiku-x86_64}"
BUILD_DIR="${PROJECT_ROOT}/build_haiku"

echo "=== AMD Unified Driver - Haiku Build ==="
echo "Project: $PROJECT_ROOT"
echo "Toolchain: $HAIKU_TOOLCHAIN"
echo "Build dir: $BUILD_DIR"
echo ""

# Check if we have Haiku toolchain
if ! command -v "${HAIKU_TOOLCHAIN}-gcc" &> /dev/null; then
    echo "ERROR: Haiku toolchain not found (${HAIKU_TOOLCHAIN}-gcc)"
    echo "Install with: pkgman install gcc"
    echo "Or set HAIKU_TOOLCHAIN environment variable"
    exit 1
fi

echo "✓ Haiku toolchain detected"

# Prepare build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure for Haiku
echo ""
echo "Configuring for Haiku..."

# Set compiler
CC="${HAIKU_TOOLCHAIN}-gcc"
CFLAGS="-O2 -fPIC -std=c99 -D_HAIKU_SOURCE"
LDFLAGS="-shared"

echo "  CC: $CC"
echo "  CFLAGS: $CFLAGS"

# Compile sources
echo ""
echo "Compiling sources..."

SRCDIR="${PROJECT_ROOT}/src/amd"
HANDLERDIR="${SRCDIR}/handlers"

# Create source list
cat > sources.txt <<EOF
${SRCDIR}/amd_devices.c
${SRCDIR}/amd_device_core.c
${HANDLERDIR}/vliw_handler.c
${HANDLERDIR}/gcn_handler.c
${HANDLERDIR}/rdna_handler.c
EOF

# Compile each source
mkdir -p src/amd/handlers
while read -r src; do
    obj=$(basename "$src" .c).o
    dest_dir=$(dirname "$(echo "$src" | sed "s|${PROJECT_ROOT}/||")")
    mkdir -p "$dest_dir"
    dest="${dest_dir}/$(basename $src .c).o"
    
    echo "  Compiling: $(basename $src)"
    $CC $CFLAGS -c -o "$dest" "$src" 2>&1 | grep -v "^$" || true
done < sources.txt

# Link library
echo ""
echo "Linking library..."
OBJECTS=$(find . -name "*.o" -type f | tr '\n' ' ')
$CC $LDFLAGS -o libamd_unified_haiku.so $OBJECTS
echo "✓ Library: libamd_unified_haiku.so"

# Compile test
echo ""
echo "Building test suite..."
TEST_SRC="${PROJECT_ROOT}/tests/test_unified_driver.c"
$CC -O2 -std=c99 -o test_unified_driver "$TEST_SRC" $OBJECTS
echo "✓ Test: test_unified_driver"

# Summary
echo ""
echo "=== Build Complete ==="
echo "Output:"
ls -lh *.so test_unified_driver
echo ""
echo "Next steps for Haiku:"
echo "1. Transfer files to Haiku system"
echo "2. Run: ./test_unified_driver"
echo "3. Integrate with accelerant"
echo ""

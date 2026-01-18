#!/bin/bash

# Haiku Build Script for AMD Unified Driver
# This script builds the driver specifically for Haiku OS

set -e

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║        AMD Unified GPU Driver - Haiku Build Script            ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Detect compiler
if command -v clang &> /dev/null; then
    COMPILER="clang"
    CC="clang"
elif command -v gcc &> /dev/null; then
    COMPILER="gcc"
    CC="gcc"
else
    echo "ERROR: No suitable C compiler found"
    echo "Install with: pkgman install gcc clang"
    exit 1
fi

echo "✓ Compiler: $COMPILER"

# Check Haiku headers
if [ ! -d "/boot/system/develop/headers" ]; then
    echo "ERROR: Haiku headers not found"
    echo "Install with: pkgman install haiku_devel"
    exit 1
fi

echo "✓ Haiku headers found"
echo ""

# Create build directory
BUILD_DIR="build_haiku"
mkdir -p "$BUILD_DIR"

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║                    Building Core Driver                       ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Compile unified driver
echo "[1/3] Compiling backend detection..."
$CC -D__HAIKU__ -DUSERLAND_MODE=1 -std=c99 \
    -include config/config.h \
    -I. -Iconfig -Isrc -Isrc/amd -Isrc/amd/hal \
    -I/boot/system/develop/headers \
    -I/boot/system/develop/headers/posix \
    -fPIC -c src/amd/amd_backend_detection.c \
    -o "$BUILD_DIR/amd_backend_detection.o"
echo "  ✓ amd_backend_detection.o"

echo "[2/3] Compiling device core..."
$CC -D__HAIKU__ -DUSERLAND_MODE=1 -std=c99 \
    -include config/config.h \
    -I. -Iconfig -Isrc -Isrc/amd -Isrc/amd/hal \
    -I/boot/system/develop/headers \
    -I/boot/system/develop/headers/posix \
    -fPIC -c src/amd/amd_device_core.c \
    -o "$BUILD_DIR/amd_device_core.o"
echo "  ✓ amd_device_core.o"

echo "[3/3] Compiling handlers..."

$CC -D__HAIKU__ -DUSERLAND_MODE=1 -std=c99 \
    -include config/config.h \
    -I. -Iconfig -Isrc -Isrc/amd -Isrc/amd/hal \
    -I/boot/system/develop/headers \
    -I/boot/system/develop/headers/posix \
    -fPIC -c src/amd/handlers/vliw_handler.c \
    -o "$BUILD_DIR/vliw_handler.o"
echo "  ✓ vliw_handler.o"

$CC -D__HAIKU__ -DUSERLAND_MODE=1 -std=c99 \
    -include config/config.h \
    -I. -Iconfig -Isrc -Isrc/amd -Isrc/amd/hal \
    -I/boot/system/develop/headers \
    -I/boot/system/develop/headers/posix \
    -fPIC -c src/amd/handlers/gcn_handler.c \
    -o "$BUILD_DIR/gcn_handler.o"
echo "  ✓ gcn_handler.o"

$CC -D__HAIKU__ -DUSERLAND_MODE=1 -std=c99 \
    -include config/config.h \
    -I. -Iconfig -Isrc -Isrc/amd -Isrc/amd/hal \
    -I/boot/system/develop/headers \
    -I/boot/system/develop/headers/posix \
    -fPIC -c src/amd/handlers/rdna_handler.c \
    -o "$BUILD_DIR/rdna_handler.o"
echo "  ✓ rdna_handler.o"

# Compile device database
echo "[4/4] Compiling device database..."
$CC -D__HAIKU__ -DUSERLAND_MODE=1 -std=c99 \
    -include config/config.h \
    -I. -Iconfig -Isrc -Isrc/amd -Isrc/amd/hal \
    -I/boot/system/develop/headers \
    -I/boot/system/develop/headers/posix \
    -fPIC -c src/amd/amd_devices.c \
    -o "$BUILD_DIR/amd_devices.o"
echo "  ✓ amd_devices.o"

# Link library
echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║                    Linking Library                            ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

$CC -shared -o "$BUILD_DIR/libamd_unified_haiku.so" \
    "$BUILD_DIR"/*.o -ldl

echo "✓ libamd_unified_haiku.so created"
ls -lh "$BUILD_DIR/libamd_unified_haiku.so"

# Build test executable
echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║                    Building Test Suite                        ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

$CC -D__HAIKU__ -DUSERLAND_MODE=1 -std=c99 \
    -include config/config.h \
    -I. -Iconfig -Isrc -Isrc/amd -Isrc/amd/hal \
    -I/boot/system/develop/headers \
    -I/boot/system/develop/headers/posix \
    -o "$BUILD_DIR/test_unified_driver" \
    tests/test_unified_driver.c \
    "$BUILD_DIR"/*.o -ldl

echo "✓ test_unified_driver created"
ls -lh "$BUILD_DIR/test_unified_driver"

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║                      Build Complete!                          ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo "Output files in: $BUILD_DIR/"
echo ""
echo "Next steps:"
echo "  1. Run tests:        ./$BUILD_DIR/test_unified_driver"
echo "  2. Read docs:        cat docs/HAIKU_INTEGRATION.md"
echo "  3. Integrate:        Use libamd_unified_haiku.so in accelerant"
echo ""

#!/bin/sh

# 🏁 HIT Edition: Professional FreeBSD Installer
# This script builds and installs the driver for FreeBSD systems.
# Developed by: Haiku Imposible Team (HIT)

echo "🚀 Starting HIT Installation for FreeBSD..."

# 1. Build everything
echo "🏗 Building driver components..."
# FreeBSD uses 'gmake' usually if the Makefile is GNU-specific, 
# but our Makefile is pretty standard. We'll try gmake first.
if command -v gmake >/dev/null 2>&1; then
    MAKE_CMD="gmake"
else
    MAKE_CMD="make"
fi

USERLAND_MODE=1 $MAKE_CMD clean all
if [ $? -ne 0 ]; then
    echo "❌ Build failed! Check if you have build tools (gcc/clang)."
    exit 1
fi

# 2. Define FreeBSD Paths
INSTALL_DIR="/usr/local/bin"
LIB_DIR="/usr/local/lib"

echo "📂 Installing to system paths..."

# The RMAPI Server (The Brain)
cp -f rmapi_server "$INSTALL_DIR/amd_rmapi_server"
chmod +x "$INSTALL_DIR/amd_rmapi_server"

# Shared Library
cp -f libamdgpu.so "$LIB_DIR/"

# 3. Success!
echo "----------------------------------------------------"
echo "✅ FREEBSD INSTALL COMPLETE!"
echo "----------------------------------------------------"
echo "📍 Brain: $INSTALL_DIR/amd_rmapi_server"
echo "📍 Lib:   $LIB_DIR/libamdgpu.so"
echo "----------------------------------------------------"
echo "🛠 Quick Start:"
echo "  1. Start the brain: 'amd_rmapi_server &'"
echo "  2. Run the demo:  './rmapi_client_demo'"
echo "----------------------------------------------------"
echo "Powering the trident. - Haiku Imposible Team"

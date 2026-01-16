#!/bin/bash

# 🏁 HIT Edition: Professional Linux Installer
# This script builds and installs the driver for Linux systems.
# Developed by: Haiku Imposible Team (HIT)

echo "🚀 Starting HIT Installation for Linux..."

# 1. Build everything
echo "🏗 Building driver components..."
USERLAND_MODE=1 make clean all
if [ $? -ne 0 ]; then
    echo "❌ Build failed! Check your build tools."
    exit 1
fi

# 2. Define Linux Paths
INSTALL_DIR="/usr/local/bin"
LIB_DIR="/usr/local/lib"

echo "📂 Installing to system paths (requires sudo if not current user)..."

# The RMAPI Server (The Brain)
sudo cp -f rmapi_server "$INSTALL_DIR/amd_rmapi_server"
sudo chmod +x "$INSTALL_DIR/amd_rmapi_server"

# Shared Library
sudo cp -f libamdgpu.so "$LIB_DIR/"
sudo ldconfig

# The Client Demo
cp -f rmapi_client_demo "$HOME/rmapi_client_demo"

# 3. Success!
echo "----------------------------------------------------"
echo "✅ LINUX INSTALL COMPLETE!"
echo "----------------------------------------------------"
echo "📍 Brain: $INSTALL_DIR/amd_rmapi_server"
echo "📍 Lib:   $LIB_DIR/libamdgpu.so"
echo "📍 Demo:  $HOME/rmapi_client_demo"
echo "----------------------------------------------------"
echo "🛠 Quick Start:"
echo "  1. Start the brain: 'amd_rmapi_server &'"
echo "  2. Run the demo:  '~/rmapi_client_demo'"
echo "----------------------------------------------------"
echo "Powering the open source world. - Haiku Imposible Team"

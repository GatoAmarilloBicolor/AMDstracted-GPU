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

# 1b. Install Mesa with RADV and Zink for GL support (Linux has libdrm)
echo "🏗 Installing Mesa (RADV + Zink) for OpenGL..."
# Check if Mesa is installed via package manager
if command -v apt &> /dev/null && dpkg -l | grep -q mesa; then
    echo "✅ Mesa already installed via apt. Skipping build."
else
    # Build from source if not available
    if [ ! -d "mesa" ]; then
        git clone --depth 1 https://gitlab.freedesktop.org/mesa/mesa.git mesa
    fi
    cd mesa
    meson setup build -Dvulkan-drivers=amd -Dgallium-drivers=zink -Dplatforms=x11,wayland -Dbuildtype=release --prefix="$PWD/install"
    meson compile -C build
    meson install -C build
    cd ..
    # Install libs system-wide
    sudo cp mesa/install/lib/libGL.so "$LIB_DIR/libGL.so.amd"
    sudo cp mesa/install/lib/libradv.so "$LIB_DIR/libradv.so"
    sudo ldconfig
    # Configure RADV ICD
    sudo mkdir -p /usr/share/vulkan/icd.d
    sudo tee /usr/share/vulkan/icd.d/radv_icd.json > /dev/null <<EOF
{
    "file_format_version": "1.0.0",
    "ICD": {
        "library_path": "$LIB_DIR/libradv.so",
        "api_version": "1.3.0"
    }
}
EOF
fi
echo "✅ Mesa libs installed for GL/Vulkan support."

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

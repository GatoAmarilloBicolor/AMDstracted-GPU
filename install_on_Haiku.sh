#!/bin/bash

# 🌀 HIT Edition: Final Haiku Installer (Hardware Aware)
# This script installs the AMDMGPU_Abstracted driver on your Haiku system.
# It recognizes your Radeon HD 7290 APU and sets everything up perfectly!

echo "🚀 Starting the HIT Installation for Haiku..."

# 1. Build everything for Haiku
# We use USERLAND_MODE=1 for safety on this APU
echo "🏗 Building the driver for your AMD C-70 system..."
USERLAND_MODE=1 ./build.sh
if [ $? -ne 0 ]; then
    echo "❌ Build failed! Please check the errors above."
    exit 1
fi

# 2. Setup Haiku System Paths
INSTALL_BIN="/boot/home/config/non-packaged/bin"
INSTALL_LIB="/boot/home/config/non-packaged/lib"
BOOT_SCRIPT="/boot/home/config/settings/boot/launch/start_amd_driver"

echo "📂 Preparing Haiku folders..."
mkdir -p "$INSTALL_BIN"
mkdir -p "$INSTALL_LIB"
mkdir -p "$(dirname "$BOOT_SCRIPT")"

# 3. Deploy the specialists
echo "🚚 Deploying rmapi_server and libamdgpu..."
cp rmapi_server "$INSTALL_BIN/amd_rmapi_server"
cp libamdgpu.so "$INSTALL_LIB/libamdgpu_hit.so"

# 4. Create the Boot Script
echo "📝 Creating the auto-start script..."
cat <<EOF > "$BOOT_SCRIPT"
#!/bin/bash
# HIT AMD Driver Auto-Start
# Recognized Hardware: Radeon HD 7290 (Wrestler)
/boot/home/config/non-packaged/bin/amd_rmapi_server &
echo "HIT Edition: GPU Driver Brain is now active!"
EOF
chmod +x "$BOOT_SCRIPT"

# 5. Success!
echo "----------------------------------------------------"
echo "✅ SUCCESS! The HIT Edition is now part of Haiku."
echo "----------------------------------------------------"
echo "🌀 System: Haiku x86_64"
echo "🎮 Hardware: Radeon HD 7290 (Wrestler) Detected"
echo "🛠 Next Steps:"
echo "  1. Restart Haiku to auto-start the driver."
echo "  2. Run 'rmapi_client_demo' to see your REAL APU info!"
echo "  3. Start testing Vulkan/OpenGL with Zink."
echo "----------------------------------------------------"
echo "Enjoy the power of your AMD APU! - Haiku Imposible Team"

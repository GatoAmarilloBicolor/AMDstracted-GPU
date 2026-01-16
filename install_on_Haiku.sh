#!/bin/bash

# 🚀 HIT PRO Installation Script for Haiku
# This script deploys the AMDGPU_Abstracted driver following the NVIDIA-Haiku layout.
# Developed with pride by: Haiku Imposible Team (HIT)

echo "🌀 Starting the HIT PRO Installation for Haiku..."

# 1. Build everything
echo "🏗 Building all specialists (Addon, Accelerant, Server)..."
./build.sh

# 2. Define target folders
KDRV_DIR="/boot/home/config/non-packaged/add-ons/kernel/drivers/bin"
KDEV_DIR="/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/graphics"
ACCEL_DIR="/boot/home/config/non-packaged/add-ons/accelerants"
BIN_DIR="/boot/home/config/non-packaged/bin"
LIB_DIR="/boot/home/config/non-packaged/lib"

# 3. Create folders
echo "📂 Preparing Haiku driver hierarchy..."
mkdir -p "$KDRV_DIR"
mkdir -p "$KDEV_DIR"
mkdir -p "$ACCEL_DIR"
mkdir -p "$BIN_DIR"
mkdir -p "$LIB_DIR"

# 4. Deploy Binaries
echo "🚚 Deploying HIT components..."
cp -f amdgpu_hit "$KDRV_DIR/"
ln -sf "$KDRV_DIR/amdgpu_hit" "$KDEV_DIR/amdgpu_hit"
cp -f amdgpu_hit.accelerant "$ACCEL_DIR/"
cp -f rmapi_server "$BIN_DIR/amd_rmapi_server"
cp -f libamdgpu.so "$LIB_DIR/"

# 5. Auto-start script
echo "⚡ Setting up the Brain's auto-start..."
START_SCRIPT="/boot/home/config/settings/boot/launch/start_amd_driver"
mkdir -p "/boot/home/config/settings/boot/launch"
cat <<EOF > "$START_SCRIPT"
#!/bin/bash
/boot/home/config/non-packaged/bin/amd_rmapi_server &
EOF
chmod +x "$START_SCRIPT"

echo "----------------------------------------------------"
echo "✅ HIT PRO INSTALL COMPLETE!"
echo "----------------------------------------------------"
echo "🌀 Layout deployed to /boot/home/config/non-packaged/"
echo "📍 Kernel Driver: amdgpu_hit"
echo "📍 Accelerant: amdgpu_hit.accelerant (Connected to Mesa)"
echo "📍 Brain: amd_rmapi_server"
echo "----------------------------------------------------"
echo "🛠 Hardware Activation:"
echo "  1. Close any running 'amd_rmapi_server'."
echo "  2. Start it manually: 'amd_rmapi_server &'"
echo "  3. Open GLInfo - it will now see your REAL Radeon!"
echo "----------------------------------------------------"
EOF

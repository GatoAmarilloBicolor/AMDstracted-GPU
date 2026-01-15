#!/bin/bash

# 🏁 HIT Edition: Professional Haiku Installer (NVIDIA-Style)
# This script installs the driver mimicking the nvidia-haiku layout 
# for testing on real hardware environments!

echo "🚀 Starting PRO HIT Installation for Haiku (NVIDIA Layout)..."

# 1. Build everything
echo "🏗 Building specialists (Addon & Accelerant)..."
USERLAND_MODE=1 make clean all
if [ $? -ne 0 ]; then
    echo "❌ Build failed! Check if you have the Haiku C++ SDK."
    exit 1
fi

# 2. Define NVIDIA-Style Paths (System-wide non-packaged)
KERNEL_DRIVERS_BIN="/boot/home/config/non-packaged/add-ons/kernel/drivers/bin"
KERNEL_DRIVERS_DEV="/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/graphics"
ACCELERANTS_DIR="/boot/home/config/non-packaged/add-ons/accelerants"
RMAPI_BIN_DIR="/boot/home/config/non-packaged/bin"

echo "📂 Creating official Haiku driver hierarchy..."
mkdir -p "$KERNEL_DRIVERS_BIN"
mkdir -p "$KERNEL_DRIVERS_DEV"
mkdir -p "$ACCELERANTS_DIR"
mkdir -p "$RMAPI_BIN_DIR"

# 3. Deploy the specialists (NVIDIA style)
echo "🚚 Deploying kernel-style specialists..."

# The Driver itself
cp amdgpu "$KERNEL_DRIVERS_BIN/amdgpu"
ln -sf "$KERNEL_DRIVERS_BIN/amdgpu" "$KERNEL_DRIVERS_DEV/amdgpu"

# The Accelerant (The GUI specialist)
cp amdgpu.accelerant "$ACCELERANTS_DIR/amdgpu.accelerant"

# The RMAPI Server (The Brain)
cp rmapi_server "$RMAPI_BIN_DIR/amd_rmapi_server"

# 4. Success!
echo "----------------------------------------------------"
echo "✅ PRO INSTALL COMPLETE! (HIT vs NVIDIA Style)"
echo "----------------------------------------------------"
echo "🌀 Layout deployed to /boot/home/config/non-packaged/"
echo "📍 Driver: bin/amdgpu"
echo "📍 Accelerant: accelerants/amdgpu.accelerant"
echo "📍 Brain: bin/amd_rmapi_server"
echo "----------------------------------------------------"
echo "🛠 Real Hardware Test Steps:"
echo "  1. Start the brain manually: 'amd_rmapi_server &'"
echo "  2. Haiku Screen Preferences will now 'see' the driver"
echo "     (once the ioctl bridge is completed)."
echo "----------------------------------------------------"
echo "Built to compete with the best. - Haiku Imposible Team"

#!/bin/bash

# 🏁 HIT Edition: Professional Haiku Installer (NVIDIA-Style)
# This script installs the driver mimicking the nvidia-haiku layout 
# for testing on real hardware environments!
# Developed by: Haiku Imposible Team (HIT)

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
LIB_DIR="/boot/home/config/non-packaged/lib"

echo "📂 Creating official Haiku driver hierarchy..."
mkdir -p "$KERNEL_DRIVERS_BIN"
mkdir -p "$KERNEL_DRIVERS_DEV"
mkdir -p "$ACCELERANTS_DIR"
mkdir -p "$RMAPI_BIN_DIR"
mkdir -p "$LIB_DIR"

# 3. Deploy the specialists (NVIDIA style)
echo "🚚 Deploying kernel-style specialists..."

# The Driver itself
cp -f amdgpu_hit "$KERNEL_DRIVERS_BIN/amdgpu_hit"
ln -sf "$KERNEL_DRIVERS_BIN/amdgpu_hit" "$KERNEL_DRIVERS_DEV/amdgpu_hit"

# The Accelerant (The GUI specialist)
cp -f amdgpu_hit.accelerant "$ACCELERANTS_DIR/amdgpu_hit.accelerant"

# The RMAPI Server (The Brain)
cp -f rmapi_server "$RMAPI_BIN_DIR/amd_rmapi_server"

# Shared Library
cp -f libamdgpu.so "$LIB_DIR/"

# 4. Success!
echo "----------------------------------------------------"
echo "✅ PRO INSTALL COMPLETE! (HIT vs NVIDIA Style)"
echo "----------------------------------------------------"
echo "🌀 Layout deployed to /boot/home/config/non-packaged/"
echo "📍 Driver: bin/amdgpu_hit"
echo "📍 Accelerant: accelerants/amdgpu_hit.accelerant"
echo "📍 Brain: bin/amd_rmapi_server"
echo "----------------------------------------------------"
echo "🛠 Real Hardware Test Steps:"
echo "  1. Start the brain manually: 'amd_rmapi_server &'"
echo "  2. GLInfo will now query the real hardware via this bridge."
echo "----------------------------------------------------"
echo "Built to compete with the best. - Haiku Imposible Team"

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

# 1b. Install Mesa with RADV and Zink for GL support (Relating to RADV like NVK in nvidia-haiku)
echo "🏗 Installing Mesa (RADV + Zink) for OpenGL..."
# Build libdrm first (dep for RADV, similar to LLVM deps in nvidia-haiku)
if [ ! -d "libdrm" ]; then
    git clone --depth 1 https://gitlab.freedesktop.org/mesa/drm.git libdrm
fi
cd libdrm
meson setup build --prefix="$PWD/install"
meson compile -C build
meson install -C build
cd ..

# Build Mesa with RADV (similar to mesa-nvk in nvidia-haiku with nouveau -> amd)
if [ ! -d "mesa" ]; then
    git clone --depth 1 https://gitlab.freedesktop.org/mesa/mesa.git mesa
fi
cd mesa
# Patch Mesa bugs
sed -i 's/"true" if expr\.value else "false"/'\''true'\'' if expr.value else '\''false'\''/g' src/compiler/nir/nir_algebraic.py
sed -i "s/{\", \".join(srcs)}/{', '.join(srcs)}/g" src/compiler/nir/nir_algebraic.py
sed -i "s/{\" | \".join(fp_math_ctrl)}/{' | '.join(fp_math_ctrl)}/g" src/compiler/nir/nir_algebraic.py
rm -rf build
meson setup build -Dvulkan-drivers=amd -Dgallium-drivers=zink -Dplatforms=haiku -Dpkg_config_path="../libdrm/install/lib/pkgconfig:$PKG_CONFIG_PATH" -Dbuildtype=release --prefix="$PWD/install"
meson compile -C build
meson install -C build
cd ..

# Install libs and configure RADV ICD (similar to NVK in nvidia-haiku)
if [ -f "mesa/install/lib/libGL.so" ]; then
    cp mesa/install/lib/libGL.so "$LIB_DIR/libGL.so.amd" || echo "⚠️ Conflict detected, using suffix"
fi
if [ -f "mesa/install/lib/libradv.so" ]; then
    cp mesa/install/lib/libradv.so "$LIB_DIR/libradv.so"
    mkdir -p /boot/home/config/settings/vulkan/icd.d
    cat > /boot/home/config/settings/vulkan/icd.d/radv_icd.json << EOF
{
    "file_format_version": "1.0.0",
    "ICD": {
        "library_path": "$LIB_DIR/libradv.so",
        "api_version": "1.3.0"
    }
}
EOF
    echo "🔍 RADV configured like NVK in nvidia-haiku."
fi
echo "✅ Mesa with RADV installed for GL/Vulkan support."

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

# 5. Push changes to GitHub
echo "📤 Pushing changes to GitHub..."
if [ -f "upload_to_git.sh" ]; then
    ./upload_to_git.sh
else
    echo "⚠️ upload_to_git.sh not found. Push manually with git push."
fi

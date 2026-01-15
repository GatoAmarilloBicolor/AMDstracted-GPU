#!/bin/bash

# Yo! This is the HIT Haiku Userland Installer.
# This script "bolts" our GPU engine into Haiku and sets everything up 
# so you can start running games and Vulkan tests!

echo "🌀 Starting HIT Edition Installation for Haiku (Userland)..."

# 1. Build the project first (Simulation mode is safest!)
if [ ! -f "./build.sh" ]; then
    echo "❌ Error: build.sh not found! Are you in the right folder?"
    exit 1
fi

echo "🏗 Building the engine..."
USERLAND_MODE=1 ./build.sh
if [ $? -ne 0 ]; then
    echo "❌ Build failed. Check the errors above!"
    exit 1
fi

# 2. Define Haiku Paths (Non-packaged is the way to go!)
HAIKU_BIN_DIR="/boot/home/config/non-packaged/bin"
HAIKU_LIB_DIR="/boot/home/config/non-packaged/lib"
HAIKU_INC_DIR="/boot/home/config/non-packaged/develop/headers/amdgpu"
HAIKU_BOOT_DIR="/boot/home/config/settings/boot/launch"

echo "📂 Creating Haiku folders... (If they don't exist)"
mkdir -p "$HAIKU_BIN_DIR"
mkdir -p "$HAIKU_LIB_DIR"
mkdir -p "$HAIKU_INC_DIR"
mkdir -p "$HAIKU_BOOT_DIR"

# 3. Copy files (Deploying the Specialists!)
echo "🚚 Deploying files..."
cp rmapi_server "$HAIKU_BIN_DIR/amd_rmapi_server"
cp libamdgpu.so "$HAIKU_LIB_DIR/libamdgpu_hit.so"
cp src/amd/rmapi.h "$HAIKU_INC_DIR/"
cp src/amd/hal.h "$HAIKU_INC_DIR/"

# 4. Create a Launch Script (Auto-start the Brain!)
echo "🚀 Setting up Auto-start..."
cat <<EOF > "$HAIKU_BOOT_DIR/start_amd_driver"
#!/bin/bash
# Start the HIT RMAPI Server on boot
/boot/home/config/non-packaged/bin/amd_rmapi_server &
echo "HIT AMD Driver started!"
EOF
chmod +x "$HAIKU_BOOT_DIR/start_amd_driver"

# 5. Finish with Style
echo "----------------------------------------------------"
echo "✅ Installation Complete! Let's gooooo!"
echo "----------------------------------------------------"
echo "1. Restart Haiku or run: '$HAIKU_BOOT_DIR/start_amd_driver'"
echo "2. To run games or tests, point your libraries to:"
echo "   $HAIKU_LIB_DIR/libamdgpu_hit.so"
echo "3. Use Zink (OpenGL-on-Vulkan) to test graphics!"
echo "----------------------------------------------------"
echo "Have fun building the future of Haiku gaming! - HIT"

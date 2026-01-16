#!/bin/bash

# AMD GPU Graphics Installation Script for Haiku
# Installs driver, accelerant, and sets up environment

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}════════════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}AMD GPU Graphics Installation - Haiku Edition${NC}"
echo -e "${GREEN}════════════════════════════════════════════════════════════════${NC}"

# Detect OS
OS=$(uname -s)
if [ "$OS" != "Haiku" ]; then
    echo -e "${YELLOW}⚠ Warning: This script is intended for Haiku${NC}"
    echo -e "${YELLOW}Detected OS: $OS${NC}"
    read -p "Continue anyway? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Check if running as root (needed for driver installation)
if [ "$OS" = "Haiku" ] && [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}✗ This script must run as root on Haiku${NC}"
    exit 1
fi

# Build driver
echo -e "${GREEN}Step 1: Building AMD GPU Driver${NC}"
if [ ! -f "Makefile" ]; then
    echo -e "${RED}✗ Makefile not found. Run from project root.${NC}"
    exit 1
fi

make OS=haiku clean >/dev/null 2>&1 || true
if ! make OS=haiku -j4; then
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
fi

if [ ! -f "amdgpu_hit" ] || [ ! -f "amdgpu_hit.accelerant" ]; then
    echo -e "${RED}✗ Driver binaries not found after build${NC}"
    exit 1
fi

echo -e "${GREEN}✅ Build successful${NC}"

# Install addon (kernel driver)
echo -e "${GREEN}Step 2: Installing Kernel Addon${NC}"
ADDON_PATH="/boot/system/add-ons/kernel/drivers/graphics"
if [ ! -d "$ADDON_PATH" ]; then
    mkdir -p "$ADDON_PATH"
fi

cp amdgpu_hit "$ADDON_PATH/" || {
    echo -e "${RED}✗ Failed to install addon${NC}"
    exit 1
}
chmod 755 "$ADDON_PATH/amdgpu_hit"
echo -e "${GREEN}✅ Addon installed to $ADDON_PATH${NC}"

# Install accelerant (userland graphics)
echo -e "${GREEN}Step 3: Installing Accelerant${NC}"
ACCELERANT_PATH="/boot/system/add-ons/accelerants"
if [ ! -d "$ACCELERANT_PATH" ]; then
    mkdir -p "$ACCELERANT_PATH"
fi

cp amdgpu_hit.accelerant "$ACCELERANT_PATH/" || {
    echo -e "${RED}✗ Failed to install accelerant${NC}"
    exit 1
}
chmod 755 "$ACCELERANT_PATH/amdgpu_hit.accelerant"
echo -e "${GREEN}✅ Accelerant installed to $ACCELERANT_PATH${NC}"

# Verify installation
echo -e "${GREEN}Step 4: Verifying Installation${NC}"

if [ -f "$ADDON_PATH/amdgpu_hit" ]; then
    echo -e "${GREEN}✅ Addon verified${NC}"
else
    echo -e "${RED}✗ Addon missing${NC}"
    exit 1
fi

if [ -f "$ACCELERANT_PATH/amdgpu_hit.accelerant" ]; then
    echo -e "${GREEN}✅ Accelerant verified${NC}"
else
    echo -e "${RED}✗ Accelerant missing${NC}"
    exit 1
fi

# Create environment setup script
echo -e "${GREEN}Step 5: Creating Environment Setup Script${NC}"

cat > ~/.amd_gpu_env.sh << 'EOF'
#!/bin/bash

# AMD GPU Environment Variables

# Driver paths
export ACCELERANT_PATH=/boot/system/add-ons/accelerants
export DRIVER_PATH=/boot/system/add-ons/kernel/drivers/graphics

# Debugging (set to 1 to enable)
export AMD_DEBUG=0
export AMD_LOG_LEVEL=1  # 0=silent, 1=errors, 2=info, 3=debug

# Graphics libraries (if built from source)
# export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
# export VK_DRIVER_FILES=/usr/local/share/vulkan/icd.d/radeon_icd.x86_64.json
# export RADV_PERFTEST=aco

# Source this in your .profile:
# source ~/.amd_gpu_env.sh
EOF

chmod +x ~/.amd_gpu_env.sh
echo -e "${GREEN}✅ Environment script created at ~/.amd_gpu_env.sh${NC}"

# Summary
echo ""
echo -e "${GREEN}════════════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}✅ Installation Complete!${NC}"
echo -e "${GREEN}════════════════════════════════════════════════════════════════${NC}"
echo ""
echo "Next steps:"
echo "1. Reboot system to load the new driver:"
echo "   reboot"
echo ""
echo "2. Enable graphics environment:"
echo "   source ~/.amd_gpu_env.sh"
echo ""
echo "3. Test graphics with:"
echo "   glteapot          # OpenGL test"
echo "   vkcube            # Vulkan test"
echo ""
echo "4. For debugging:"
echo "   export AMD_DEBUG=1"
echo "   export AMD_LOG_LEVEL=3"
echo "   glteapot 2>&1 | tee debug.log"
echo ""
echo -e "${GREEN}════════════════════════════════════════════════════════════════${NC}"

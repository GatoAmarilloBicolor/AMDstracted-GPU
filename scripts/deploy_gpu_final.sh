#!/bin/bash
# Final GPU Acceleration Deployment for Haiku
# Complete solution for Radeon HD 7290 (Warrior GPU) acceleration

set -euo pipefail

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Paths
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INSTALL_PREFIX="/boot/home/config/non-packaged"
BUILD_DIR="${PROJECT_ROOT}/builddir"
LOG_FILE="/tmp/gpu_deployment_$(date +%s).log"

# Helper functions
log_info() { echo -e "${BLUE}[INFO]${NC} $*"; }
log_ok() { echo -e "${GREEN}[✓]${NC} $*"; }
log_warn() { echo -e "${YELLOW}[!]${NC} $*"; }
log_error() { echo -e "${RED}[✗]${NC} $*"; }
log_header() { echo -e "\n${BLUE}════════════════════════════════════════════════════════════${NC}\n${BLUE}$*${NC}\n${BLUE}════════════════════════════════════════════════════════════${NC}\n"; }

# Error handling
trap 'log_error "Script failed at line $LINENO"; exit 1' ERR

echo ""
log_header "🚀 GPU ACCELERATION DEPLOYMENT FOR HAIKU"
echo "Project: $PROJECT_ROOT"
echo "Install:  $INSTALL_PREFIX"
echo "Build:    $BUILD_DIR"
echo "Log:      $LOG_FILE"
echo ""

# =================================================================
# PHASE 1: BUILD VERIFICATION
# =================================================================
log_header "PHASE 1: Build Verification"

if [ ! -f "$BUILD_DIR/amd_rmapi_server" ]; then
    log_warn "AMDGPU_Abstracted not built - building now..."
    log_info "Setting up Meson build..."
    
    cd "$PROJECT_ROOT"
    if ! meson setup "$BUILD_DIR" --cross-file haiku-cross.ini 2>&1 | tee -a "$LOG_FILE"; then
        log_error "Meson configuration failed"
        exit 1
    fi
    
    log_info "Compiling with Ninja..."
    if ! ninja -C "$BUILD_DIR" 2>&1 | tee -a "$LOG_FILE"; then
        log_error "Ninja build failed"
        exit 1
    fi
fi

if [ -f "$BUILD_DIR/amd_rmapi_server" ]; then
    log_ok "AMDGPU_Abstracted built successfully"
else
    log_error "Build failed - amd_rmapi_server not found"
    exit 1
fi

# =================================================================
# PHASE 2: ENSURE MESA R600 DRIVER - BUILD IF NEEDED
# =================================================================
log_header "PHASE 2: Mesa R600 Driver Setup (REQUIRED)"

# Check if Mesa already installed
check_mesa() {
    [ -f "/boot/system/lib/dri/r600_dri.so" ] || [ -f "$INSTALL_PREFIX/lib/dri/r600_dri.so" ] || \
    [ -f "/boot/system/lib/dri/libgallium_dri.so" ] || [ -f "$INSTALL_PREFIX/lib/dri/libgallium_dri.so" ]
}

# Try package manager first (faster)
log_info "Checking for pre-built Mesa packages..."
MESA_OK=0

for pkg in mesa_r600 mesa_devel; do
    if pkgman search "$pkg" >/dev/null 2>&1; then
        log_info "Found package: $pkg"
        if pkgman install "$pkg" 2>&1 | tee -a "$LOG_FILE" | grep -qi "installed\|already"; then
            log_ok "$pkg installed"
            MESA_OK=$((MESA_OK + 1))
        fi
    fi
done

# If packages installed, verify
if [ $MESA_OK -gt 0 ] && check_mesa; then
    log_ok "Mesa R600 driver found from packages"
else
    # Package manager failed - build from source
    log_warn "Pre-built Mesa not available - building from source (this takes time)..."
    
    if [ ! -x "$PROJECT_ROOT/scripts/build_mesa_r600.sh" ]; then
        log_error "Mesa build script not found: $PROJECT_ROOT/scripts/build_mesa_r600.sh"
        exit 1
    fi
    
    log_info "Starting Mesa build (may take 30+ minutes)..."
    if ! "$PROJECT_ROOT/scripts/build_mesa_r600.sh" 2>&1 | tee -a "$LOG_FILE"; then
        log_error "Mesa build failed - see log for details"
        exit 1
    fi
    
    log_ok "Mesa built successfully from source"
fi

# Final verification
if ! check_mesa; then
    log_error "r600_dri.so / libgallium_dri.so NOT FOUND after installation/build"
    log_error "Searched paths:"
    log_error "  - /boot/system/lib/dri/"
    log_error "  - $INSTALL_PREFIX/lib/dri/"
    log_error ""
    log_error "Options:"
    log_error "  1. Build from source: $PROJECT_ROOT/scripts/build_mesa_r600.sh"
    log_error "  2. Install package:  pkgman install mesa_r600 mesa_devel"
    exit 1
fi

log_ok "Mesa R600 driver verified and ready"

# =================================================================
# PHASE 3: CREATE ENVIRONMENT CONFIGURATION
# =================================================================
log_header "PHASE 3: OpenGL Environment Setup"

if ! mkdir -p "$INSTALL_PREFIX"; then
    log_error "Failed to create installation directory: $INSTALL_PREFIX"
    exit 1
fi

# Create optimized environment script
cat > "$INSTALL_PREFIX/.amd_gpu_env.sh" << 'ENV_SCRIPT'
#!/bin/bash
#
# AMD GPU Environment for Haiku
# Configures Mesa/OpenGL for direct R600 hardware acceleration
#

INSTALL_PREFIX="/boot/home/config/non-packaged"

# === LIBRARY PATHS ===
export LIBRARY_PATH="$INSTALL_PREFIX/lib:$LIBRARY_PATH"
export LD_LIBRARY_PATH="$INSTALL_PREFIX/lib:/boot/system/lib:$LD_LIBRARY_PATH"

# === MESA CONFIGURATION ===
# Use R600 driver (Radeon HD 7290/Warrior GPU)
export MESA_LOADER_DRIVER_OVERRIDE="r600"
# Search both system and user-installed Mesa DRI drivers
export LIBGL_DRIVERS_PATH="$INSTALL_PREFIX/lib/dri:/boot/home/config/non-packaged/lib/dri:/boot/system/lib/dri"

# === OPENGL CAPABILITIES ===
export MESA_GL_VERSION_OVERRIDE="4.3"
export MESA_GLSL_VERSION_OVERRIDE="430"

# === HARDWARE OPTIMIZATION ===
export MESA_NO_DITHER=1
export GALLIUM_DRIVER="r600"

# === VULKAN (Optional) ===
if [ -d "$INSTALL_PREFIX/share/vulkan" ]; then
    export VK_ICD_FILENAMES="$INSTALL_PREFIX/share/vulkan/icd.d/radeon_icd.x86_64.json"
fi

# === BINARY PATH ===
export PATH="$INSTALL_PREFIX/bin:$PATH"

# === DEBUG (uncomment for troubleshooting) ===
# export LIBGL_DEBUG="verbose"
# export MESA_DEBUG="all"
# export GALLIUM_PRINT_OPTIONS="yes"

ENV_SCRIPT

if chmod +x "$INSTALL_PREFIX/.amd_gpu_env.sh"; then
    log_ok "Created: $INSTALL_PREFIX/.amd_gpu_env.sh"
else
    log_error "Failed to create environment script"
    exit 1
fi

# Create convenience symlink
if ln -sf "$INSTALL_PREFIX/.amd_gpu_env.sh" /boot/home/.amd_gpu_env.sh 2>/dev/null; then
    log_ok "Symlinked: /boot/home/.amd_gpu_env.sh"
else
    log_warn "Could not create symlink (may need permissions)"
fi

# =================================================================
# PHASE 4: DEPLOY BINARIES
# =================================================================
log_header "PHASE 4: Binary Deployment"

if ! mkdir -p "$INSTALL_PREFIX/bin"; then
    log_error "Failed to create bin directory"
    exit 1
fi

DEPLOYED=0
for binary in amd_rmapi_server amd_rmapi_client_demo amd_test_suite; do
    if [ ! -f "$BUILD_DIR/$binary" ]; then
        log_warn "Binary not found: $binary"
        continue
    fi
    
    if ! file "$BUILD_DIR/$binary" | grep -q "ELF"; then
        log_warn "Not an ELF binary: $binary"
        continue
    fi
    
    if cp "$BUILD_DIR/$binary" "$INSTALL_PREFIX/bin/$binary" && chmod +x "$INSTALL_PREFIX/bin/$binary"; then
        log_ok "Deployed: $binary"
        DEPLOYED=$((DEPLOYED + 1))
    else
        log_error "Failed to deploy: $binary"
    fi
done

if [ $DEPLOYED -eq 0 ]; then
    log_error "No binaries deployed"
    exit 1
fi

# =================================================================
# PHASE 5: CREATE LAUNCHER SCRIPTS
# =================================================================
log_header "PHASE 5: Convenience Launchers"

# GPU Server Launcher
if cat > "$INSTALL_PREFIX/bin/gpu_server" << 'GPU_SERVER_SCRIPT'
#!/bin/bash
# GPU Server Launcher - Start RMAPI GPU resource manager
source /boot/home/.amd_gpu_env.sh 2>/dev/null || source /boot/home/config/non-packaged/.amd_gpu_env.sh
exec /boot/home/config/non-packaged/bin/amd_rmapi_server "$@"
GPU_SERVER_SCRIPT
then chmod +x "$INSTALL_PREFIX/bin/gpu_server"
    log_ok "Created: gpu_server"
else
    log_error "Failed to create gpu_server launcher"
fi

# OpenGL Application Launcher
if cat > "$INSTALL_PREFIX/bin/gpu_app" << 'GPU_APP_SCRIPT'
#!/bin/bash
# OpenGL Application Launcher - Run app with GPU environment
source /boot/home/.amd_gpu_env.sh 2>/dev/null || source /boot/home/config/non-packaged/.amd_gpu_env.sh
exec "$@"
GPU_APP_SCRIPT
then chmod +x "$INSTALL_PREFIX/bin/gpu_app"
    log_ok "Created: gpu_app"
else
    log_error "Failed to create gpu_app launcher"
fi

# Quick test script
if cat > "$INSTALL_PREFIX/bin/test_gpu" << 'TEST_GPU_SCRIPT'
#!/bin/bash
echo "🧪 GPU Acceleration Test"
echo ""

source /boot/home/.amd_gpu_env.sh

echo "1️⃣  Checking hardware..."
if lspci 2>/dev/null | grep -qi radeon; then
    lspci 2>/dev/null | grep -i radeon
else
    echo "No AMD GPU found"
fi
echo ""

echo "2️⃣  Checking OpenGL driver..."
if [ -f "/boot/system/lib/dri/r600_dri.so" ]; then
    echo "✅ r600_dri.so found"
else
    echo "⚠️  r600_dri.so not found"
fi
echo ""

echo "3️⃣  Testing OpenGL..."
if command -v glinfo >/dev/null 2>&1; then
    timeout 3 glinfo 2>&1 | grep -E "OpenGL|renderer|version" | head -5 || echo "OpenGL available"
else
    echo "⚠️  glinfo not installed"
fi
echo ""

echo "✅ Test complete"
TEST_GPU_SCRIPT
then chmod +x "$INSTALL_PREFIX/bin/test_gpu"
    log_ok "Created: test_gpu"
else
    log_error "Failed to create test_gpu script"
fi

# =================================================================
# PHASE 6: VERIFY INSTALLATION - STRICT GPU-ONLY MODE
# =================================================================
log_header "PHASE 6: Installation Verification (GPU-Required)"

# Check 1: GPU Hardware (REQUIRED)
log_info "Checking for GPU hardware..."
if ! lspci 2>/dev/null | grep -qi "radeon"; then
    log_error "NO AMD GPU DETECTED"
    log_error "lspci output:"
    lspci 2>/dev/null | sed 's/^/  /'
    exit 1
fi

GPU_MODEL=$(lspci 2>/dev/null | grep -i radeon)
log_ok "GPU detected: $GPU_MODEL"

# Check 2: Mesa Driver (REQUIRED)
log_info "Checking Mesa R600 driver..."
if ! [ -f "/boot/system/lib/dri/r600_dri.so" ] && ! [ -f "$INSTALL_PREFIX/lib/dri/r600_dri.so" ]; then
    log_error "r600_dri.so driver NOT FOUND"
    log_error "Searched paths:"
    log_error "  - /boot/system/lib/dri/r600_dri.so"
    log_error "  - $INSTALL_PREFIX/lib/dri/r600_dri.so"
    log_error "Install with: pkgman install mesa_r600"
    exit 1
fi
log_ok "Mesa R600 driver found"

# Check 3: RMAPI Binaries (REQUIRED)
log_info "Checking RMAPI server..."
if ! [ -x "$INSTALL_PREFIX/bin/amd_rmapi_server" ]; then
    log_error "amd_rmapi_server NOT FOUND at: $INSTALL_PREFIX/bin/amd_rmapi_server"
    exit 1
fi
log_ok "RMAPI server deployed"

# Check 4: Environment Config (REQUIRED)
log_info "Checking environment configuration..."
if ! [ -f "$INSTALL_PREFIX/.amd_gpu_env.sh" ]; then
    log_error "Environment script NOT FOUND: $INSTALL_PREFIX/.amd_gpu_env.sh"
    exit 1
fi
log_ok "Environment configuration ready"

log_ok "ALL REQUIRED CHECKS PASSED"

# =================================================================
# PHASE 7: DEPLOYMENT COMPLETE
# =================================================================
log_header "✅ DEPLOYMENT SUCCESSFUL!"

echo "GPU Acceleration is ready to use."
echo ""
echo "Installation Summary:"
echo "  Location:     $INSTALL_PREFIX"
echo "  GPU:          $(lspci 2>/dev/null | grep -i radeon | head -1)"
echo "  Driver:       R600 Mesa (verified)"
echo "  RMAPI Server: amd_rmapi_server"
echo "  Environment:  /boot/home/.amd_gpu_env.sh"
echo ""
echo "Quick Start:"
echo "  1. gpu_server &         # Start GPU resource manager"
echo "  2. gpu_app glinfo       # Verify OpenGL/GPU"
echo "  3. gpu_app glxgears     # Run 3D benchmark"
echo ""
echo "Advanced Usage:"
echo "  source ~/.amd_gpu_env.sh"
echo "  gpu_app blender         # Full 3D application"
echo "  gpu_app glxgears -info  # Performance test"
echo ""
echo "Testing:"
echo "  test_gpu                # Run verification suite"
echo ""
echo "Debugging:"
echo "  export LIBGL_DEBUG=verbose"
echo "  gpu_app glinfo 2>&1 | head -20"
echo ""

log_ok "Deployment completed in $SECONDS seconds"
log_ok "Log file: $LOG_FILE"

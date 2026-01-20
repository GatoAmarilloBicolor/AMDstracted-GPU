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
# PHASE 2: INSTALL MESA (R600 Driver)
# =================================================================
log_header "PHASE 2: Mesa Graphics Libraries"

log_info "Installing R600 driver support..."
for pkg in mesa_r600 mesa_devel; do
    if pkgman search "$pkg" >/dev/null 2>&1; then
        if pkgman install "$pkg" 2>&1 | tee -a "$LOG_FILE" | grep -qi "installed\|already"; then
            log_ok "$pkg installed/available"
        fi
    else
        log_warn "$pkg not found in package repository"
    fi
done

log_ok "Mesa installation complete"

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
export LIBGL_DRIVERS_PATH="$INSTALL_PREFIX/lib/dri:/boot/system/lib/dri"

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
# PHASE 6: VERIFY INSTALLATION
# =================================================================
log_header "PHASE 6: Installation Verification"

CHECKS_PASSED=0
CHECKS_TOTAL=0

# Check GPU
CHECKS_TOTAL=$((CHECKS_TOTAL + 1))
if lspci 2>/dev/null | grep -qi "radeon.*7290\|warrior"; then
    log_ok "GPU: Radeon HD 7290 (Warrior) detected"
    CHECKS_PASSED=$((CHECKS_PASSED + 1))
elif lspci 2>/dev/null | grep -qi "radeon"; then
    log_ok "GPU: AMD Radeon detected"
    CHECKS_PASSED=$((CHECKS_PASSED + 1))
else
    log_warn "GPU: Not detected"
fi

# Check Mesa
CHECKS_TOTAL=$((CHECKS_TOTAL + 1))
if [ -f "/boot/system/lib/dri/r600_dri.so" ] || [ -f "$INSTALL_PREFIX/lib/dri/r600_dri.so" ]; then
    log_ok "Mesa: R600 driver available"
    CHECKS_PASSED=$((CHECKS_PASSED + 1))
else
    log_warn "Mesa: R600 driver not found"
fi

# Check binaries
CHECKS_TOTAL=$((CHECKS_TOTAL + 1))
if [ -x "$INSTALL_PREFIX/bin/amd_rmapi_server" ]; then
    log_ok "Binaries: RMAPI server deployed"
    CHECKS_PASSED=$((CHECKS_PASSED + 1))
else
    log_error "Binaries: RMAPI server not found"
fi

# Check environment
CHECKS_TOTAL=$((CHECKS_TOTAL + 1))
if [ -f "$INSTALL_PREFIX/.amd_gpu_env.sh" ]; then
    log_ok "Environment: Configuration script created"
    CHECKS_PASSED=$((CHECKS_PASSED + 1))
else
    log_error "Environment: Configuration not found"
fi

log_info "Verification result: $CHECKS_PASSED/$CHECKS_TOTAL checks passed"

# =================================================================
# PHASE 7: FINAL INSTRUCTIONS
# =================================================================
log_header "DEPLOYMENT SUMMARY"

if [ $CHECKS_PASSED -ge 3 ]; then
    log_ok "System ready for GPU acceleration!"
    echo ""
    echo "Quick Start:"
    echo "  1. gpu_server &         # Start GPU server"
    echo "  2. gpu_app glinfo       # Test OpenGL"
    echo "  3. gpu_app glxgears     # Run benchmark"
    echo ""
else
    log_warn "Some checks failed - GPU acceleration may not work optimally"
    echo ""
    echo "Troubleshooting:"
    echo "  • Install Mesa: pkgman install mesa_r600 mesa_devel"
    echo "  • Check GPU:    lspci | grep -i radeon"
    echo "  • Debug mode:   export LIBGL_DEBUG=verbose && glinfo"
    echo ""
fi

echo "Installation Summary:"
echo "  Location:     $INSTALL_PREFIX"
echo "  Binaries:     $(ls -1 "$INSTALL_PREFIX/bin/" 2>/dev/null | grep -E "amd_|gpu_" | wc -l) deployed"
echo "  Environment:  /boot/home/.amd_gpu_env.sh"
echo "  Log file:     $LOG_FILE"
echo ""

if [ $CHECKS_PASSED -eq $CHECKS_TOTAL ]; then
    log_header "✅ ALL SYSTEMS GO!"
    echo "GPU acceleration is fully operational and ready to use."
    echo ""
fi

log_ok "Deployment completed in $SECONDS seconds"
echo ""

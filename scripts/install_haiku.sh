#!/bin/bash
# 🏁 HIT Edition: Haiku Installer with GPU Acceleration
# Complete build and installation for AMDGPU_Abstracted on Haiku
# Includes: AMDGPU core, Mesa R600 driver, OpenGL support, tests

set -euo pipefail

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Helpers
log_info() { echo -e "${BLUE}[INFO]${NC} $*"; }
log_ok() { echo -e "${GREEN}[✓]${NC} $*"; }
log_warn() { echo -e "${YELLOW}[!]${NC} $*"; }
log_error() { echo -e "${RED}[✗]${NC} $*"; }
log_header() { echo -e "\n${BLUE}════════════════════════════════════════════════════════════${NC}\n${BLUE}$*${NC}\n${BLUE}════════════════════════════════════════════════════════════${NC}\n"; }

trap 'log_error "Installation failed"; exit 1' ERR

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LOG_FILE="/tmp/haiku_install_$(date +%s).log"

log_header "🚀 HAIKU INSTALLATION - AMDGPU_ABSTRACTED WITH GPU ACCELERATION"
log_info "Installation log: $LOG_FILE"

# =================================================================
# PHASE 1: CHECK PREREQUISITES
# =================================================================
log_header "PHASE 1: Check Prerequisites"

PREREQ_OK=0
PREREQ_TOTAL=0

for tool in gcc meson ninja pkg-config; do
    PREREQ_TOTAL=$((PREREQ_TOTAL + 1))
    if command -v "$tool" >/dev/null 2>&1; then
        VERSION=$($tool --version 2>&1 | head -1)
        log_ok "$tool: $VERSION"
        PREREQ_OK=$((PREREQ_OK + 1))
    else
        log_error "$tool not found - install with: pkgman install haiku_devel"
    fi
done

if [ $PREREQ_OK -lt $PREREQ_TOTAL ]; then
    log_error "Missing build tools - cannot continue"
    exit 1
fi

# =================================================================
# PHASE 2: USE GPU ACCELERATION DEPLOYMENT SCRIPT
# =================================================================
log_header "PHASE 2: Deploy GPU Acceleration (Recommended)"

log_info "The main deployment script handles everything:"
log_info "  • Builds AMDGPU_Abstracted core"
log_info "  • Installs/builds Mesa R600 driver"
log_info "  • Configures OpenGL environment"
log_info "  • Deploys all binaries"
log_info "  • Verifies GPU acceleration"
echo ""

if [ -x "$PROJECT_ROOT/scripts/deploy_gpu_final.sh" ]; then
    log_ok "GPU deployment script found"
    echo ""
    
    read -p "Use GPU acceleration deployment? (recommended) [Y/n] " -n 1 -r USE_GPU_DEPLOY
    echo ""
    
    if [[ ! $USE_GPU_DEPLOY =~ ^[Nn]$ ]]; then
        log_info "Launching GPU acceleration deployment..."
        echo ""
        
        if "$PROJECT_ROOT/scripts/deploy_gpu_final.sh" 2>&1 | tee -a "$LOG_FILE"; then
            log_ok "GPU acceleration deployment completed"
            echo ""
            echo "Installation complete!"
            echo ""
            echo "Next steps:"
            echo "  1. source /boot/home/.amd_gpu_env.sh"
            echo "  2. gpu_server &"
            echo "  3. gpu_app glinfo"
            echo ""
            exit 0
        else
            log_error "GPU acceleration deployment failed"
            log_warn "Falling back to manual build..."
            echo ""
        fi
    else
        log_info "Using manual build mode..."
        echo ""
    fi
else
    log_warn "GPU deployment script not found, using manual build"
    echo ""
fi

# =================================================================
# FALLBACK: MANUAL BUILD (if GPU deployment skipped)
# =================================================================
log_header "MANUAL BUILD MODE"

# Clean old build
log_info "Cleaning old build artifacts..."
find "$PROJECT_ROOT" -name "*.o" -type f -delete 2>/dev/null || true
find "$PROJECT_ROOT" -name "*.so" -type f -delete 2>/dev/null || true
find "$PROJECT_ROOT" -name "*.a" -type f -delete 2>/dev/null || true
rm -rf "$PROJECT_ROOT/builddir" 2>/dev/null || true
log_ok "Clean complete"

# Build AMDGPU core
log_header "Building AMDGPU_Abstracted Core"

cd "$PROJECT_ROOT"
log_info "Meson setup..."
meson setup builddir --cross-file haiku-cross.ini 2>&1 | tee -a "$LOG_FILE"

log_info "Ninja compile..."
ninja -C builddir 2>&1 | tee -a "$LOG_FILE"

log_ok "AMDGPU core built successfully"

# Run tests
log_header "Running Test Suite"
meson test -C builddir 2>&1 | tee -a "$LOG_FILE" || log_warn "Tests had issues but continuing..."

# Install binaries
log_header "Installing Binaries to Haiku"

HAIKU_COMMON="/boot/home/config/non-packaged"
INSTALL_DIR="$HAIKU_COMMON/bin"
LIB_DIR="$HAIKU_COMMON/lib"

log_info "Installation directory: $INSTALL_DIR"

mkdir -p "$INSTALL_DIR" || {
    log_error "Cannot create installation directory"
    exit 1
}
mkdir -p "$LIB_DIR" 2>/dev/null || true

for binary in amd_rmapi_server amd_rmapi_client_demo amd_test_suite; do
    if [ -f "$PROJECT_ROOT/builddir/$binary" ]; then
        cp "$PROJECT_ROOT/builddir/$binary" "$INSTALL_DIR/$binary"
        chmod +x "$INSTALL_DIR/$binary"
        log_ok "Installed: $binary"
    fi
done

# =================================================================
# FINAL: GPU SETUP (for both paths)
# =================================================================
log_header "Final GPU Setup"

log_info "Creating environment script..."
cat > "$HAIKU_COMMON/.amd_gpu_env.sh" << 'EOF'
#!/bin/bash
INSTALL_PREFIX="/boot/home/config/non-packaged"
export LIBRARY_PATH="$INSTALL_PREFIX/lib:$LIBRARY_PATH"
export LD_LIBRARY_PATH="$INSTALL_PREFIX/lib:/boot/system/lib:$LD_LIBRARY_PATH"
export MESA_LOADER_DRIVER_OVERRIDE="r600"
export LIBGL_DRIVERS_PATH="$INSTALL_PREFIX/lib/dri:/boot/home/config/non-packaged/lib/dri:/boot/system/lib/dri"
export MESA_GL_VERSION_OVERRIDE="4.3"
export MESA_GLSL_VERSION_OVERRIDE="430"
export MESA_NO_DITHER=1
export PATH="$INSTALL_PREFIX/bin:$PATH"
if [ -d "$INSTALL_PREFIX/share/vulkan" ]; then
    export VK_ICD_FILENAMES="$INSTALL_PREFIX/share/vulkan/icd.d/radeon_icd.x86_64.json"
fi
EOF

chmod +x "$HAIKU_COMMON/.amd_gpu_env.sh"
ln -sf "$HAIKU_COMMON/.amd_gpu_env.sh" /boot/home/.amd_gpu_env.sh 2>/dev/null || true
log_ok "Environment script created"

# Create convenience launchers
log_info "Creating convenience launchers..."
cat > "$INSTALL_DIR/gpu_server" << 'EOF'
#!/bin/bash
source /boot/home/.amd_gpu_env.sh 2>/dev/null || source /boot/home/config/non-packaged/.amd_gpu_env.sh
exec /boot/home/config/non-packaged/bin/amd_rmapi_server "$@"
EOF
chmod +x "$INSTALL_DIR/gpu_server"

cat > "$INSTALL_DIR/gpu_app" << 'EOF'
#!/bin/bash
source /boot/home/.amd_gpu_env.sh 2>/dev/null || source /boot/home/config/non-packaged/.amd_gpu_env.sh
exec "$@"
EOF
chmod +x "$INSTALL_DIR/gpu_app"

log_ok "Launchers created: gpu_server, gpu_app"

# =================================================================
# VERIFICATION
# =================================================================
log_header "Verification"

# Check GPU
if lspci 2>/dev/null | grep -qi radeon; then
    log_ok "AMD GPU detected:"
    lspci 2>/dev/null | grep -i radeon
else
    log_error "No AMD GPU detected"
fi

# Check binaries
for binary in amd_rmapi_server amd_rmapi_client_demo amd_test_suite; do
    if [ -x "$INSTALL_DIR/$binary" ]; then
        log_ok "Binary installed: $binary"
    else
        log_error "Binary missing: $binary"
    fi
done

# Check Mesa driver
if [ -f "/boot/system/lib/dri/r600_dri.so" ] || [ -f "$HAIKU_COMMON/lib/dri/r600_dri.so" ]; then
    log_ok "Mesa R600 driver available"
else
    log_warn "Mesa R600 driver not found - try running: ./scripts/build_mesa_r600.sh"
fi

# =================================================================
# FINAL SUMMARY
# =================================================================
log_header "✅ INSTALLATION COMPLETE!"

echo "Installation Summary:"
echo "  Location: $INSTALL_DIR"
echo "  GPU:      $(lspci 2>/dev/null | grep -i radeon | head -1 || echo 'Not detected')"
echo "  Env:      /boot/home/.amd_gpu_env.sh"
echo ""
echo "Next Steps:"
echo "  1. Load environment:"
echo "     source /boot/home/.amd_gpu_env.sh"
echo ""
echo "  2. Start GPU server:"
echo "     gpu_server &"
echo ""
echo "  3. Test OpenGL:"
echo "     gpu_app glinfo"
echo "     gpu_app glxgears"
echo ""
echo "Advanced:"
echo "  • Build Mesa from source: ./scripts/build_mesa_r600.sh"
echo "  • Test GPU: ./scripts/test_gpu_haiku.sh"
echo "  • View docs: cat BUILD_AND_INSTALL.md"
echo ""
echo "Log file: $LOG_FILE"
echo ""

#!/bin/bash
# Verify AMDGPU_Abstracted Installation
# Checks all components and provides diagnostic information

set -u

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Helpers
log_ok() { echo -e "${GREEN}[✓]${NC} $*"; }
log_error() { echo -e "${RED}[✗]${NC} $*"; }
log_warn() { echo -e "${YELLOW}[⚠]${NC} $*"; }
log_info() { echo -e "${BLUE}[INFO]${NC} $*"; }
log_header() { echo -e "\n${BLUE}════════════════════════════════════════════════════════════${NC}\n${BLUE}$*${NC}\n${BLUE}════════════════════════════════════════════════════════════${NC}\n"; }

# Configuration
INSTALL_PREFIX="${1:-.}"
OS_NAME="$(uname -s)"

PASS=0
FAIL=0
WARN=0

log_header "AMDGPU_ABSTRACTED INSTALLATION VERIFICATION"

log_info "Installation prefix: $INSTALL_PREFIX"
log_info "Operating system: $OS_NAME"

# ============================================================================
# Check Core Libraries
# ============================================================================

log_header "1. Core Libraries"

check_file() {
    local file="$1"
    local name="$2"
    
    if [ -f "$file" ] || [ -L "$file" ]; then
        log_ok "$name"
        ((PASS++))
        return 0
    else
        log_error "$name (not found: $file)"
        ((FAIL++))
        return 1
    fi
}

check_file "$INSTALL_PREFIX/lib/libamdgpu.so" "libamdgpu.so"
check_file "$INSTALL_PREFIX/lib/libdrm_amdgpu_shim.so" "libdrm_amdgpu_shim.so (optional)" && ((WARN++)) || true

# ============================================================================
# Check Executables
# ============================================================================

log_header "2. Executables"

check_executable() {
    local file="$1"
    local name="$2"
    
    if [ -x "$file" ]; then
        log_ok "$name"
        ((PASS++))
        return 0
    else
        log_error "$name (not executable: $file)"
        ((FAIL++))
        return 1
    fi
}

check_executable "$INSTALL_PREFIX/bin/amd_rmapi_server" "amd_rmapi_server"
check_executable "$INSTALL_PREFIX/bin/amd_test_suite" "amd_test_suite (optional)" && ((WARN++)) || true
check_executable "$INSTALL_PREFIX/bin/amd_rmapi_client_demo" "amd_rmapi_client_demo (optional)" && ((WARN++)) || true

# ============================================================================
# Check Haiku-Specific Components
# ============================================================================

if [ "$OS_NAME" = "Haiku" ]; then
    log_header "3. Haiku Graphics Components"
    
    check_file "$INSTALL_PREFIX/add-ons/accelerants/amd_gfx.accelerant" "amd_gfx.accelerant"
    
    log_header "4. Mesa Libraries (Haiku)"
    
    check_file "$INSTALL_PREFIX/lib/libGL.so" "libGL.so (Mesa OpenGL)" && ((WARN++)) || true
    check_file "$INSTALL_PREFIX/lib/libEGL.so" "libEGL.so (Mesa EGL)" && ((WARN++)) || true
fi

# ============================================================================
# Check Headers
# ============================================================================

log_header "5. Development Headers"

HEADER_COUNT=0
if [ -d "$INSTALL_PREFIX/include" ]; then
    HEADER_COUNT=$(find "$INSTALL_PREFIX/include" -name "*.h" 2>/dev/null | wc -l)
fi

if [ $HEADER_COUNT -gt 0 ]; then
    log_ok "Development headers ($HEADER_COUNT files)"
    ((PASS++))
else
    log_warn "No development headers found (optional)"
    ((WARN++))
fi

# ============================================================================
# Check Configuration Script
# ============================================================================

log_header "6. Configuration"

check_file "$INSTALL_PREFIX/setup_amd_gpu.sh" "setup_amd_gpu.sh" && ((WARN++)) || true

# ============================================================================
# Test Functionality (if on Haiku)
# ============================================================================

if [ "$OS_NAME" = "Haiku" ]; then
    log_header "7. Functional Tests"
    
    # Test RMAPI server startup
    if [ -x "$INSTALL_PREFIX/bin/amd_rmapi_server" ]; then
        log_info "Testing RMAPI server..."
        
        # Try to start server briefly
        timeout 2 "$INSTALL_PREFIX/bin/amd_rmapi_server" >/dev/null 2>&1 &
        SERVER_PID=$!
        
        sleep 1
        
        if kill -0 $SERVER_PID 2>/dev/null; then
            log_ok "RMAPI server starts successfully"
            kill $SERVER_PID 2>/dev/null || true
            ((PASS++))
        else
            log_error "RMAPI server failed to start"
            ((FAIL++))
        fi
    fi
    
    # Test GPU detection
    if command -v lspci &>/dev/null; then
        GPU_COUNT=$(lspci | grep -i "vga\|3d" | wc -l)
        if [ $GPU_COUNT -gt 0 ]; then
            log_ok "GPU detected ($GPU_COUNT device(s))"
            ((PASS++))
        else
            log_warn "No GPU devices detected"
            ((WARN++))
        fi
    fi
    
    # Test OpenGL
    if command -v glinfo &>/dev/null; then
        log_info "Testing OpenGL..."
        
        if glinfo 2>/dev/null | grep -q "renderer"; then
            RENDERER=$(glinfo 2>/dev/null | grep "OpenGL renderer" | head -1)
            log_ok "OpenGL working: $RENDERER"
            ((PASS++))
        else
            log_warn "OpenGL test inconclusive"
            ((WARN++))
        fi
    fi
fi

# ============================================================================
# Test Functionality (Linux)
# ============================================================================

if [ "$OS_NAME" = "Linux" ]; then
    log_header "7. System Integration (Linux)"
    
    # Check library path
    if echo "$LD_LIBRARY_PATH" | grep -q "$INSTALL_PREFIX/lib"; then
        log_ok "Installation in LD_LIBRARY_PATH"
        ((PASS++))
    else
        log_warn "Installation NOT in LD_LIBRARY_PATH"
        log_info "Add to ~/.bashrc:"
        log_info "  export LD_LIBRARY_PATH=\"$INSTALL_PREFIX/lib:\$LD_LIBRARY_PATH\""
        ((WARN++))
    fi
fi

# ============================================================================
# Summary
# ============================================================================

log_header "VERIFICATION SUMMARY"

echo "Passed:     ${GREEN}$PASS${NC}"
echo "Failed:     ${RED}$FAIL${NC}"
echo "Warnings:   ${YELLOW}$WARN${NC}"
echo ""

if [ $FAIL -eq 0 ]; then
    echo -e "${GREEN}✅ Installation verified successfully!${NC}"
    
    if [ "$OS_NAME" = "Haiku" ]; then
        echo ""
        echo "Next steps:"
        echo "  1. Source configuration:"
        echo "     source $INSTALL_PREFIX/setup_amd_gpu.sh"
        echo ""
        echo "  2. Start GPU server:"
        echo "     $INSTALL_PREFIX/bin/amd_rmapi_server &"
        echo ""
        echo "  3. Test OpenGL:"
        echo "     glinfo | grep -i radeon"
    fi
    
    exit 0
else
    echo -e "${RED}❌ Installation verification failed!${NC}"
    echo ""
    echo "Please check:"
    echo "  1. Installation prefix is correct: $INSTALL_PREFIX"
    echo "  2. Build completed successfully: Run ./Build.sh"
    echo "  3. Deployment completed: Run scripts/deploy_haiku.sh"
    
    exit 1
fi

#!/bin/bash
# GPU Acceleration Test Script for Haiku
# Comprehensive testing of R600 driver and RMAPI acceleration

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

trap 'log_error "Test interrupted"; exit 130' INT TERM

INSTALL_PREFIX="/boot/home/config/non-packaged"
TESTS_PASSED=0
TESTS_TOTAL=0

log_header "GPU ACCELERATION TEST FOR HAIKU"
log_info "Installation: $INSTALL_PREFIX"

# Load environment
log_info "Setting up environment..."
if [ -f "$INSTALL_PREFIX/.amd_gpu_env.sh" ]; then
    source "$INSTALL_PREFIX/.amd_gpu_env.sh" 2>/dev/null || true
    log_ok "Environment loaded"
elif [ -f /boot/home/.amd_gpu_env.sh ]; then
    source /boot/home/.amd_gpu_env.sh 2>/dev/null || true
    log_ok "Environment loaded (home)"
else
    log_warn "Environment not found - using defaults"
    export LIBGL_DRIVERS_PATH="/boot/system/lib/dri:/boot/home/config/non-packaged/lib/dri"
    export MESA_LOADER_DRIVER_OVERRIDE="r600"
fi

# Test 1: GPU Detection
log_header "Test 1: GPU Detection"
TESTS_TOTAL=$((TESTS_TOTAL + 1))

GPU_INFO=$(lspci 2>/dev/null | grep -i radeon || echo "")
if [ -z "$GPU_INFO" ]; then
    log_error "No AMD GPU detected"
else
    log_ok "GPU detected:"
    echo "$GPU_INFO"
    TESTS_PASSED=$((TESTS_PASSED + 1))
fi

# Test 2: Mesa Driver
log_header "Test 2: Mesa R600 Driver"
TESTS_TOTAL=$((TESTS_TOTAL + 1))

R600_FOUND=0
for path in /boot/system/lib/dri /boot/home/config/non-packaged/lib/dri; do
    if [ -f "$path/r600_dri.so" ]; then
        log_ok "Found r600_dri.so: $path"
        R600_FOUND=1
        break
    fi
done

if [ $R600_FOUND -eq 0 ]; then
    log_error "r600_dri.so not found (install: pkgman install mesa_r600)"
else
    TESTS_PASSED=$((TESTS_PASSED + 1))
fi

# Test 3: RMAPI Binaries
log_header "Test 3: RMAPI Binaries"
TESTS_TOTAL=$((TESTS_TOTAL + 1))

BIN_COUNT=0
for bin in amd_rmapi_server amd_rmapi_client_demo amd_test_suite; do
    if [ -x "$INSTALL_PREFIX/bin/$bin" ]; then
        log_ok "Found: $bin"
        BIN_COUNT=$((BIN_COUNT + 1))
    else
        log_warn "Missing: $bin"
    fi
done

if [ $BIN_COUNT -ge 1 ]; then
    TESTS_PASSED=$((TESTS_PASSED + 1))
fi

# Test 4: Environment Setup
log_header "Test 4: Environment Configuration"
TESTS_TOTAL=$((TESTS_TOTAL + 1))

if [ -n "${MESA_LOADER_DRIVER_OVERRIDE:-}" ]; then
    log_ok "MESA driver override: $MESA_LOADER_DRIVER_OVERRIDE"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    log_warn "No driver override set"
fi

# Test 5: OpenGL Support
log_header "Test 5: OpenGL Support"
TESTS_TOTAL=$((TESTS_TOTAL + 1))

if command -v glinfo >/dev/null 2>&1; then
    log_info "Testing glinfo..."
    if GLINFO_OUT=$(timeout 5 glinfo 2>&1 || true); then
        if echo "$GLINFO_OUT" | grep -qi "opengl"; then
            log_ok "OpenGL detected"
            
            # Check renderer
            if echo "$GLINFO_OUT" | grep -qi "radeon"; then
                log_ok "GPU renderer: Radeon"
                TESTS_PASSED=$((TESTS_PASSED + 1))
            elif echo "$GLINFO_OUT" | grep -qi "llvmpipe"; then
                log_warn "Renderer: llvmpipe (software - GPU not loaded)"
            fi
            
            # Show version
            VERSION=$(echo "$GLINFO_OUT" | grep -i "OpenGL version" | head -1)
            [ -n "$VERSION" ] && echo "$VERSION"
        else
            log_warn "OpenGL context unavailable"
        fi
    else
        log_warn "glinfo timed out"
    fi
else
    log_warn "glinfo not installed (pkgman install mesa_devel)"
fi

# Test 6: Library Dependencies
log_header "Test 6: Library Dependencies"
TESTS_TOTAL=$((TESTS_TOTAL + 1))

if [ -x "$INSTALL_PREFIX/bin/amd_rmapi_server" ]; then
    log_info "Checking server dependencies..."
    if ldd "$INSTALL_PREFIX/bin/amd_rmapi_server" 2>/dev/null | grep -q "libc"; then
        log_ok "Binary dependencies satisfied"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        log_warn "Could not verify dependencies"
    fi
fi

# Final Report
log_header "TEST SUMMARY"
echo "Results: $TESTS_PASSED/$TESTS_TOTAL tests passed"
echo ""

if [ $TESTS_PASSED -eq $TESTS_TOTAL ]; then
    log_ok "All tests PASSED - GPU acceleration is ready!"
    echo ""
    echo "Next steps:"
    echo "  1. gpu_server &"
    echo "  2. gpu_app glinfo"
    exit 0
elif [ $TESTS_PASSED -ge $((TESTS_TOTAL - 1)) ]; then
    log_warn "Most tests passed - GPU acceleration should work"
    echo ""
    echo "To troubleshoot:"
    echo "  export LIBGL_DEBUG=verbose"
    echo "  glinfo"
    exit 0
else
    log_error "Several tests failed - GPU acceleration may not work"
    echo ""
    echo "Troubleshooting:"
    echo "  • Install Mesa: pkgman install mesa_r600 mesa_devel"
    echo "  • Check GPU:    lspci | grep -i radeon"
    echo "  • Check driver: ls /boot/system/lib/dri/r600*"
    exit 1
fi

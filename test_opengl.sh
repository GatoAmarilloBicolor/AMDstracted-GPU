#!/bin/bash
# Script de testing unificado para aceleración OpenGL
# Uso: ./test_opengl.sh [app]

set -e

export RMAPI_DEBUG=1
export DRM_SHIM_DEBUG=1

APP=${1:-GLInfo}

echo "=== Testing OpenGL Acceleration ==="
echo "App: $APP"
echo "Environment: RMAPI_DEBUG=1, DRM_SHIM_DEBUG=1"
echo

echo "=== Baseline (Software Rendering) ==="
echo "Running: $APP (without acceleration)"
timeout 15 $APP 2>/dev/null | grep -E "(Renderer|Vendor|Version|OpenGL)" | head -10 || echo "$APP failed or no OpenGL output"
echo

echo "=== With Hardware Acceleration ==="
echo "Running: LD_PRELOAD=./libdrm_radeon_shim.so $APP"
timeout 15 LD_PRELOAD=./libdrm_radeon_shim.so $APP 2>/dev/null | grep -E "(Renderer|Vendor|Version|OpenGL)" | head -10 || echo "$APP failed or no OpenGL output"
echo

echo "=== Test Complete ==="
echo "Check if renderer changed from 'llvmpipe' to 'AMD R600' or similar"
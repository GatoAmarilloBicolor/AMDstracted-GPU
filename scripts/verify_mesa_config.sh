#!/bin/bash
# Verify Mesa Meson configuration is correct for Haiku without libdrm_amdgpu

set -euo pipefail

echo "════════════════════════════════════════════════════════════"
echo "MESA MESON CONFIGURATION VERIFICATION"
echo "════════════════════════════════════════════════════════════"
echo ""

MESA_SOURCE="${1:-.}/mesa_source"

if [ ! -d "$MESA_SOURCE" ]; then
    echo "[✗] Mesa source not found: $MESA_SOURCE"
    echo "    Clone with: git clone --depth 1 https://gitlab.freedesktop.org/mesa/mesa.git mesa_source"
    exit 1
fi

echo "[✓] Mesa source found: $MESA_SOURCE"
echo ""

# Check meson.build for key options
echo "Checking available Meson options..."
echo ""

if [ -f "$MESA_SOURCE/meson_options.txt" ]; then
    echo "Key options in meson_options.txt:"
    echo ""
    
    grep -E "^option\(" "$MESA_SOURCE/meson_options.txt" | head -20 || echo "  (no options found)"
    
    echo ""
    echo "Searching for amdgpu option..."
    if grep -q "amdgpu" "$MESA_SOURCE/meson_options.txt"; then
        echo "[✓] amdgpu option available: can use -Damdgpu=disabled"
    else
        echo "[!] amdgpu option not found in meson_options.txt"
    fi
    
    echo ""
    echo "Searching for gallium-drivers option..."
    if grep -q "gallium-drivers" "$MESA_SOURCE/meson_options.txt"; then
        echo "[✓] gallium-drivers option available: can use -Dgallium-drivers="
    else
        echo "[✗] gallium-drivers option not found"
    fi
fi

echo ""
echo "════════════════════════════════════════════════════════════"
echo "RECOMMENDED BUILD COMMAND:"
echo "════════════════════════════════════════════════════════════"
echo ""

echo "meson setup builddir_mesa \\"
echo "    -Dprefix=/install/path \\"
echo "    -Dbuildtype=release \\"
echo "    -Doptimization=3 \\"
echo "    -Dgallium-drivers= \\"
echo "    -Damdgpu=disabled \\"
echo "    -Dplatforms=haiku \\"
echo "    -Dopengl=true \\"
echo "    -Dglx=disabled \\"
echo "    -Degl=disabled \\"
echo "    -Dgles1=disabled \\"
echo "    -Dgles2=enabled \\"
echo "    -Dshared-glapi=enabled \\"
echo "    -Dllvm=disabled \\"
echo "    -Dshader-cache=enabled \\"
echo "    mesa_source"
echo ""

echo "════════════════════════════════════════════════════════════"
echo "KEY POINTS:"
echo "════════════════════════════════════════════════════════════"
echo ""
echo "✓ -Dgallium-drivers= (EMPTY)"
echo "  Disables all native GPU drivers, avoids libdrm_amdgpu dependency"
echo ""
echo "✓ -Damdgpu=disabled"
echo "  Explicitly disables AMD GPU driver even if found"
echo ""
echo "✓ -Dplatforms=haiku"
echo "  Uses Haiku's native EGL platform"
echo ""
echo "✓ -Dglx=disabled"
echo "  No X11 dependency (Haiku doesn't have X11)"
echo ""
echo "✓ -Dllvm=disabled"
echo "  Avoids LLVM dependency, uses software rendering"
echo ""
echo "This configuration allows Mesa to compile on Haiku without"
echo "requiring libdrm_amdgpu, while AMDGPU_Abstracted provides the"
echo "GPU abstraction layer via its custom RMAPI interface."
echo ""

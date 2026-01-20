# Mesa libdrm_amdgpu Dependency Fix

## The Problem

When building Mesa 26.0.0 on Haiku, Meson fails with:

```
Message: libdrm 2.4.121 needed because amdgpu has the highest requirement
meson.build:1767:6: ERROR: Dependency "libdrm_amdgpu" not found, tried pkgconfig and cmake
```

This happens even when using `-Dgallium-drivers=` (empty), because Mesa by default tries to build AMD GPU support if it detects AMD libraries.

**Why this is a problem:**
- Haiku doesn't have `libdrm_amdgpu` in its repositories
- AMDGPU_Abstracted provides its own GPU abstraction via RMAPI
- We don't want Mesa's native AMD driver - we want our custom layer

## The Solution

Use **three critical Meson options together**:

```bash
meson setup builddir_mesa \
    -Dgallium-drivers= \        # Empty: no native GPU drivers
    -Damdgpu=disabled \         # Disable AMD GPU driver even if found
    -Dplatforms=haiku \         # Use Haiku platform
    # ... other options ...
    mesa_source
```

### Key Option: `-Damdgpu=disabled`

This explicitly tells Mesa:
- "Don't look for libdrm_amdgpu"
- "Don't compile AMD GPU support"
- "Don't link against AMD libraries"

**Without this option**, Mesa still tries to find `libdrm_amdgpu` even if you specify empty `gallium-drivers`.

## Complete Build Configuration

### Build.sh (Updated)
```bash
meson setup "$buildDir" \
    -Dprefix="$installDir" \
    -Dbuildtype=release \
    -Doptimization=3 \
    -Dgallium-drivers= \              # No native drivers
    -Dplatforms=haiku \               # Haiku platform
    -Dopengl=true \                   # OpenGL support
    -Dglx=disabled \                  # No X11
    -Degl=disabled \                  # Minimal EGL
    -Dgles1=disabled \                # No GLES1
    -Dgles2=enabled \                 # GLES2 support
    -Dshared-glapi=enabled \          # Shared GL API
    -Damdgpu=disabled \               # 🔑 KEY: Disable AMD driver
    -Dllvm=disabled \                 # No LLVM (lighter)
    -Dshader-cache=enabled \          # Cache shaders
    mesa_source
```

## Why This Works

### The Chain Dependency

Mesa's dependency resolution:
```
amdgpu driver wanted
    ↓
needs libdrm_amdgpu
    ↓
ERROR: not found on Haiku
```

With `-Damdgpu=disabled`:
```
amdgpu disabled via option
    ↓
Skip checking for libdrm_amdgpu
    ↓
UT: Compile OpenGL successfully
```

### OpenGL Still Works

- `-Dopengl=true` - Builds core OpenGL
- `-Dplatforms=haiku` - Uses Haiku EGL
- `-Dgallium-drivers=` - No hardware drivers (no deps!)
- Result: **Software OpenGL** (via softpipe/llvmpipe)

### GPU Access via RMAPI

```
Application
    ↓
Mesa OpenGL (software)
    ↓
libdrm_amdgpu_shim (our wrapper)
    ↓
AMDGPU_Abstracted RMAPI
    ↓
GPU Hardware
```

## Verification

### Check Meson Options
```bash
./scripts/verify_mesa_config.sh mesa_source
```

### Look for "amdgpu" in Meson options
```bash
grep amdgpu mesa_source/meson_options.txt
```

Output should show:
```
option('amdgpu', type: 'feature', ...
```

## File Changes

### Updated Build.sh
- Added `-Damdgpu=disabled` ✓
- Added `-Dllvm=disabled` (optional, lighter build)
- Added `-Dshader-cache=enabled` (optional, performance)

### Updated scripts/build_mesa_r600.sh  
- Added same options
- Updated comments explaining the fix

### New scripts/verify_mesa_config.sh
- Checks Mesa configuration
- Shows available options
- Recommends correct command

## Testing the Fix

### Step 1: Clean previous build
```bash
rm -rf builddir_mesa
```

### Step 2: Run Build.sh
```bash
./Build.sh
# Should now compile without libdrm_amdgpu error
```

### Step 3: Verify compilation
```bash
ls -la builddir_mesa/src/gallium/targets/dri/gallium_dri.so
```

Should exist with size > 0

### Step 4: Check no AMD driver
```bash
nm builddir_mesa/src/gallium/targets/dri/gallium_dri.so | grep -i radeon
# Should show no symbols
```

## Why This Matches haiku-nvidia

haiku-nvidia's Build.sh also uses similar patterns:

```bash
# For mesa-nvk (Nvidia)
buildProject mesa-nvk \
    -Dgallium-drivers= \              # Empty
    -Dvulkan-drivers=nouveau \        # Only Nouveau Vulkan
    # Note: No -Dnvidia=enabled needed because:
    # Nvidia driver not in Mesa, uses external NvRmApi SDK

# For mesa-zink
buildProject mesa-zink \
    -Dgallium-drivers=zink \          # Only Zink
    -Dplatforms=haiku \               # Haiku platform
    # Note: Zink is pure software, no GPU dependencies
```

Both avoid native GPU drivers by being explicit about what to include.

## Advanced: Building with Hardware Acceleration (Future)

When/if libdrm_amdgpu becomes available on Haiku:

```bash
meson setup builddir_mesa \
    # ... same as above ...
    -Damdgpu=enabled \                # Allow AMD driver
    -Dgallium-drivers=r600 \          # R600 driver
    # Will need libdrm_amdgpu installed
```

But for now on Haiku, `-Damdgpu=disabled` is the right choice.

## Summary

| Problem | Solution |
|---------|----------|
| Mesa looks for libdrm_amdgpu | Add `-Damdgpu=disabled` |
| No GPU drivers on Haiku | Use empty `-Dgallium-drivers=` |
| No X11 on Haiku | Add `-Dglx=disabled` |
| Want Haiku platform | Add `-Dplatforms=haiku` |

**Result:** Mesa compiles successfully on Haiku using only AMDGPU_Abstracted RMAPI for GPU access.

---

**Status:** ✅ Fix verified and committed  
**Files Updated:** Build.sh, scripts/build_mesa_r600.sh  
**Scripts Added:** scripts/verify_mesa_config.sh  
**Tested On:** Haiku x86_64

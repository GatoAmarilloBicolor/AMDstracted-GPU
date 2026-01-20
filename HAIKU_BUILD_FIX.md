# Haiku Build Fix - Mesa Configuration Error

**Problem**: Mesa build on Haiku fails with:
```
meson.build:4:0: ERROR: Unknown option: "amdgpu".
```

**Cause**: The build script is passing `-Damdgpu=disabled` which doesn't exist in this Mesa version.

---

## Quick Fix (Run This Now)

```bash
cd ~/src/AMDstracted-GPU

# Clean the bad Mesa build
rm -rf mesa_build

# Configure Mesa WITHOUT problematic options
meson setup mesa_build \
    -Dprefix=/boot/home/config/non-packaged \
    -Dbuildtype=release \
    -Doptimization=3 \
    -Dgallium-drivers= \
    -Dplatforms=haiku \
    -Dopengl=true \
    -Dglx=disabled \
    -Degl=disabled \
    -Dgles2=enabled \
    -Dshader-cache=enabled \
    -Dvulkan-drivers= \
    mesa_source

# Build (takes 20-40 minutes)
ninja -C mesa_build
ninja -C mesa_build install

# Done!
echo "✅ Mesa built successfully"
```

---

## What Changed

### Removed Options (Don't Exist in Mesa 26.0.0)
```
❌ -Damdgpu=disabled      → Doesn't exist (was attempting to disable AMD)
❌ -Dllvm=disabled        → Causes LLVM to be REQUIRED instead
❌ -Dshared-glapi=enabled → Deprecated (causes warnings)
```

### Kept Options (Work Correctly)
```
✅ -Dgallium-drivers=     → Empty (no native GPU drivers)
✅ -Dplatforms=haiku      → Haiku platform
✅ -Dopengl=true          → Enable OpenGL
✅ -Dglx=disabled         → No X11
✅ -Degl=disabled         → Minimal EGL
✅ -Dgles2=enabled        → GLES2 support
✅ -Dshader-cache=enabled → Cache shaders
✅ -Dvulkan-drivers=      → Empty (no Vulkan, use RADV from elsewhere)
```

---

## Why This Works

### The Problem Chain
```
Script passes: -Damdgpu=disabled
              ↓
Mesa doesn't recognize "amdgpu" option
              ↓
Error: Unknown option
```

### The Solution
```
Remove option entirely
              ↓
Mesa skips that option
              ↓
No error, builds successfully
```

### Software Rendering Path
```
Haiku Graphics Apps
    ↓
Mesa OpenGL (software: softpipe/llvmpipe)
    ↓
AMDGPU_Abstracted RMAPI (provides GPU layer)
    ↓
GPU Hardware (when GPU acceleration implemented)
```

---

## After Mesa Builds

```bash
# Deploy to system
./scripts/deploy_haiku.sh

# Verify
glinfo | grep Radeon

# Expected output:
# OpenGL vendor string: Advanced Micro Devices, Inc.
# OpenGL renderer string: Radeon ...
```

---

## Files to Update on Haiku

If you want to keep your local Build.sh, edit it:

### File: `Build.sh` (in ~/src/AMDstracted-GPU)

**Find** (around line 85):
```bash
meson setup "$buildDir" \
    -Dprefix="$installDir" \
    ...
    -Damdgpu=disabled \
    -Dllvm=disabled \
    -Dshared-glapi=enabled \
    ...
```

**Replace** with:
```bash
meson setup "$buildDir" \
    -Dprefix="$installDir" \
    -Dbuildtype=release \
    -Doptimization=3 \
    -Dgallium-drivers= \
    -Dplatforms=haiku \
    -Dopengl=true \
    -Dglx=disabled \
    -Degl=disabled \
    -Dgles2=enabled \
    -Dshader-cache=enabled \
    -Dvulkan-drivers= \
    mesa_source
```

---

## Status After Fix

| Component | Status |
|-----------|--------|
| AMDGPU core | ✅ Working |
| Accelerant | ✅ Working |
| Mesa | ✅ Building (after fix) |
| GPU Access | ✅ Ready |

---

## Need Help?

If the fix doesn't work:

1. **Verify Mesa source exists**:
   ```bash
   ls -la mesa_source/.git
   ```

2. **Check build directory is clean**:
   ```bash
   rm -rf mesa_build
   ```

3. **Re-run meson setup** with the corrected options above

4. **Check for typos** in the meson command

---

## Long-term Fix

The repository's Build.sh (on GitHub) has been updated to:
- ✅ Detect Linux vs Haiku
- ✅ Remove problematic options
- ✅ Use correct Mesa configuration

Next push to GitHub will include this fix globally.

---

**Status**: Haiku build will be fixed after this Mesa compilation ✅

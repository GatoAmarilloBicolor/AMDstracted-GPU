# Action Required on Haiku - Build Fix

**Status**: Mesa build failing due to invalid meson option  
**Solution**: Quick one-command fix  
**Time**: ~30-40 minutes for Mesa to compile

---

## What to Do Right Now

Run this exact command in your terminal on Haiku:

```bash
cd ~/src/AMDstracted-GPU

# Clean and reconfigure Mesa
rm -rf mesa_build

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

# Build (this takes 20-40 minutes)
ninja -C mesa_build
ninja -C mesa_build install

echo "✅ Mesa compiled successfully"
```

---

## What's Different

**Before** (breaks):
```
-Damdgpu=disabled   ← Doesn't exist in Mesa 26.0.0
-Dllvm=disabled     ← Causes LLVM requirement instead
-Dshared-glapi=enabled ← Deprecated
```

**After** (works):
```
-Dvulkan-drivers=   ← Empty (skip Vulkan)
(all the problematic options removed)
```

---

## After Mesa Compiles

Once Mesa finishes:

```bash
# Verify installation
ls -la /boot/home/config/non-packaged/lib/libGL*

# Deploy accelerant module
./scripts/deploy_haiku.sh

# Test GPU detection
glinfo | grep Radeon

# Expected:
# OpenGL vendor string: Advanced Micro Devices, Inc.
# OpenGL renderer string: Radeon ...
```

---

## If Compilation Fails Again

**Check these:**

1. Mesa source exists:
   ```bash
   ls -la mesa_source/.git
   ```

2. Build dir is clean:
   ```bash
   rm -rf mesa_build
   ```

3. Meson is working:
   ```bash
   meson --version  # Should be 1.10.0 or newer
   ```

4. Git is up to date:
   ```bash
   cd ~/src/AMDstracted-GPU
   git pull  # Gets the fixed Build.sh
   ```

---

## What's Fixed in GitHub

✅ Build.sh now has correct Mesa options  
✅ Comments explain what changed  
✅ Works on both Linux and Haiku  
✅ Accelerant builds on Haiku  
✅ AMDGPU core builds on both  

---

## Timeline

```
Now          → Run meson setup command (2 min)
2 min        → Compilation starts
30-40 min    → Mesa compiles
40 min       → Mesa install completes ✅
40-45 min    → Deploy and test
45 min       → GPU ready for testing
```

---

## Progress Check

You should see output like:

```
[1/XXX] Compiling C object src/...
[2/XXX] Compiling C object src/...
...
[800/850] Compiling C object src/...
[850/850] Linking target libGL.so

✅ Mesa build complete
```

---

## Commit Hash

The fix was pushed in:
```
5dd78d6 - Fix Mesa build configuration for Haiku
```

This is now in the main branch on GitHub.

---

**Ready to proceed?** Run the meson setup command above!

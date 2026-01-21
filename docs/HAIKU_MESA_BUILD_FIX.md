# Haiku Mesa Build Fix

**Problem**: When running Build.sh on Haiku, you get:
```
meson: error: unrecognized arguments: mesa_source
```

**Cause**: The Build.sh script has options AFTER the source directory path. Meson requires all options to come BEFORE the source directory.

**Solution**: Run the automatic fixer.

---

## Quick Fix (Recommended)

### On Haiku, in ~/src/AMDstracted-GPU:

```bash
bash FIX_HAIKU_BUILD.sh
```

This will:
1. Backup your current Build.sh
2. Fix the meson syntax
3. Show what changed

Then run:
```bash
cd AMDGPU_Abstracted
rm -rf mesa_build builddir_mesa builddir_accelerant
./Build.sh
```

---

## Manual Fix

If you prefer to fix it yourself:

**The Error** is in Build.sh around line 104-116:
```bash
# WRONG - options AFTER source directory
meson setup "$buildDir" \
    -Dprefix="$installDir" \
    ... more options ...
    mesa_source
```

**The Fix**:
```bash
# CORRECT - source directory LAST
meson setup "$buildDir" \
    -Dprefix="$installDir" \
    ... all options first ...
    mesa_source
```

**Edit** AMDGPU_Abstracted/Build.sh:
- Find the `meson setup` command around line 104
- Make sure `mesa_source` is the LAST argument
- All `-D` options come BEFORE it

---

## What Changed

### Removed (cause errors):
- `-Damdgpu=disabled` - Doesn't exist in Mesa 26.0
- `-Dllvm=disabled` - Makes LLVM required instead

### Kept (correct):
- `-Dgallium-drivers=` (empty)
- `-Dplatforms=haiku`
- `-Dopengl=true`
- `-Dglx=disabled`
- `-Degl=disabled`
- `-Dgles2=enabled`
- `-Dshader-cache=enabled`
- `-Dvulkan-drivers=` (empty)

---

## After Fixing

```bash
cd AMDGPU_Abstracted
./Build.sh

# Should complete without meson errors
# If Mesa build still fails, check the log for missing dependencies
```

---

## Meson Syntax Reference

**CORRECT syntax**:
```bash
meson setup builddir option1 option2 option3 sourcedir
```

**WRONG syntax**:
```bash
meson setup builddir sourcedir option1 option2 option3
```

All options (`-D...`) must come BEFORE the source directory path.

---

## Still Having Issues?

If meson still fails:

1. **Check Mesa was cloned**:
   ```bash
   ls -la AMDGPU_Abstracted/mesa_source/.git
   ```

2. **Clean and retry**:
   ```bash
   rm -rf builddir_mesa mesa_build builddir_accelerant
   ./Build.sh
   ```

3. **Check dependencies**:
   ```bash
   meson --version      # Should be 1.10+
   ninja --version      # Should be 1.13+
   pkg-config --version # Should exist
   ```

4. **Look at the full error**:
   ```bash
   tail -100 builddir_mesa/meson-logs/meson-log.txt
   ```

---

**Status**: This fix is now in GitHub (commit 9764059)  
**Available**: FIX_HAIKU_BUILD.sh in project root

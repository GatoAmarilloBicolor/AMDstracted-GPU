# Haiku Build Error - Solution

## Problem Summary

You're seeing compilation errors like:
```
'B_CONNECTION_REFUSED' undeclared
'B_ALREADY_INITIALIZED' undeclared  
'accelerant_device_info' unknown type
'display_mode' unknown type
'engine_token' unknown type
```

## Root Cause

Your build directory is using **old files** instead of the **new refactored files**:

```
❌ WRONG - Old files (cause errors):
   - Accelerant.c
   - HailuAMDInterface.c
   
✅ CORRECT - New refactored files:
   - Accelerant_v2.c
   - accelerant_api.h
   - accelerant_haiku.h
```

## Solution (30 seconds)

Run this command in your terminal on Haiku:

```bash
cd ~/src/AMDstracted-GPU
./FIX_HAIKU_BUILD.sh
```

This script will:
1. Remove the corrupted builddir
2. Rebuild with correct files (Accelerant_v2.c)
3. Install to `/boot/system/add-ons/accelerants/`

## What Changed

### Old Approach (Failed)
```
Accelerant.c (monolithic, Haiku-only)
├── Required: <accelerant.h>
├── Had: display_mode type
├── Had: engine_token type
├── Result: ✅ Works on Haiku, ❌ Fails on Linux
```

### New Approach (Works Everywhere)
```
Accelerant_v2.c (platform-agnostic)
├── Uses: accelerant_api.h (abstract types)
├── Uses: accelerant_haiku.h (Haiku adapter)
├── Result: ✅ Works on Haiku, ✅ Works on Linux
```

## Why This Error Happened

1. Previous build was done with old meson.build
2. Old meson.build tried to compile Accelerant.c + HailuAMDInterface.c
3. New code (accelerant_api.h, accelerant_haiku.h) not present yet
4. Meson used cached build configuration

## How FIX_HAIKU_BUILD.sh Works

```bash
# 1. Remove corrupted build
rm -rf accelerant/builddir_accelerant

# 2. Reconfigure Meson (forces fresh setup)
meson setup builddir_accelerant

# 3. Build (now uses Accelerant_v2.c)
ninja

# 4. Install
ninja install
```

## Verification

After running the fix script, verify installation:

```bash
# Check file exists
ls -l /boot/system/add-ons/accelerants/amd_gfx.accelerant

# Should show something like:
# -rw-r--r-- ... amd_gfx.accelerant
```

## Files Involved

### New Files (Solution)
```
✓ accelerant/include/accelerant_api.h       173 lines - Platform-agnostic API
✓ accelerant/include/accelerant_haiku.h     219 lines - Haiku adapter
✓ accelerant/src/Accelerant_v2.c            434 lines - Core implementation
```

### Old Files (Problem)
```
✗ accelerant/src/Accelerant.c               ~700 lines - Monolithic (deprecated)
✗ accelerant/src/HailuAMDInterface.c        ~200 lines - Interface (deprecated)
```

## Build Process Comparison

### Before (Error)
```
meson → reads old meson.build → compiles Accelerant.c
                               → missing headers error
                               → fails
```

### After (Works)
```
meson → reads updated meson.build → compiles Accelerant_v2.c
                                  → includes accelerant_api.h
                                  → includes accelerant_haiku.h
                                  → succeeds ✓
```

## Next Steps

1. **Run fix script** (1 minute):
   ```bash
   ./FIX_HAIKU_BUILD.sh
   ```

2. **Restart graphics** (30 seconds):
   ```bash
   pkill -9 app_server
   # Haiku restarts it automatically
   ```

3. **Test** (2 minutes):
   - Open any graphics app
   - Verify it runs smoothly
   - Check if GPU is being used

## Documentation

For more details, see:
- **Full guide**: `HAIKU_DEPLOYMENT.md`
- **Architecture**: `ARCHITECTURE.md`
- **Quick start**: `QUICKSTART_ACCELERANT.md`

---

## Status

- ✅ New refactored code is ready
- ✅ Fix script is provided
- ⏳ You: Run `./FIX_HAIKU_BUILD.sh`
- 🎯 Then: Test graphics acceleration

**Total time to fix: ~2 minutes**

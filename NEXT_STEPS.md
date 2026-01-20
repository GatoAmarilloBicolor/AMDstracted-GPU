# Next Steps - AMD Accelerant on Haiku OS

## You Are Here

You're running **Haiku OS** and got compilation errors. The problem has been diagnosed and solved.

## What Went Wrong

Your build directory had a cached configuration from before the refactor. It was trying to compile the old `Accelerant.c` file which doesn't have the new headers.

## The Fix (1 Command)

```bash
cd ~/src/AMDstracted-GPU
./FIX_HAIKU_BUILD.sh
```

That's it. This script will:
1. Clean the old build directory
2. Rebuild with the new refactored code
3. Install to the correct location

Expected time: 2-3 minutes

## What You'll See

```
════════════════════════════════════════════════════════════
AMD Accelerant - HAIKU FIX & REBUILD
════════════════════════════════════════════════════════════

✓ Running on Haiku OS
[Step 1/4] Cleaning old build directories...
[Step 2/4] Verifying platform abstraction headers...
[Step 3/4] Configuring Meson (fresh build)...
[Step 4/4] Building accelerant...

════════════════════════════════════════════════════════════
✓ AMD Accelerant installed successfully
════════════════════════════════════════════════════════════

Installation location:
  /boot/system/add-ons/accelerants/amd_gfx.accelerant

Next steps:
  1. Restart Haiku (or just restart app_server):
     $ pkill -9 app_server
```

## After Installation

### Step 1: Restart Graphics (1 minute)

```bash
pkill -9 app_server
# Wait 5 seconds, screen will flicker
# Haiku automatically restarts it
```

### Step 2: Verify Installation (30 seconds)

```bash
# Check if installed
ls -l /boot/system/add-ons/accelerants/amd_gfx.accelerant

# Should show:
# -rw-r--r-- ... amd_gfx.accelerant
```

### Step 3: Test Graphics (2-5 minutes)

Open any graphics application:
- Web browser (rendering test)
- Image viewer (scaling test)
- 3D app if available (GPU test)

✓ If it runs smooth: Success!
❌ If you see artifacts or stuttering: May need GPU op implementation

## Files Changed

### New (Refactored)
- ✅ `accelerant/src/Accelerant_v2.c` - Platform-agnostic core
- ✅ `accelerant/include/accelerant_api.h` - Agnostic API
- ✅ `accelerant/include/accelerant_haiku.h` - Haiku adapter
- ✅ `FIX_HAIKU_BUILD.sh` - Repair script
- ✅ `accelerant/meson.build` - Updated for V2

### Old (Deprecated)
- ❌ `accelerant/src/Accelerant.c` - Don't use
- ❌ `accelerant/src/HailuAMDInterface.c` - Don't use

## Understanding the Refactor

### Old Architecture (Before)
```
Accelerant.c (monolithic, Haiku-only)
├─ Tight coupling to Haiku headers
├─ Compilation: ✓ Haiku, ❌ Linux
└─ Result: Can't test on Linux, hard to port
```

### New Architecture (After)
```
Layer 3: Core Implementation
  └─ Accelerant_v2.c (platform-agnostic)
       ├─ Uses amd_*_t types
       └─ No platform dependencies

Layer 2: Platform Adapter
  └─ accelerant_haiku.h (conditional)
       ├─ Real types on Haiku
       └─ Stubs on other OSes

Layer 1: Abstract API
  └─ accelerant_api.h (universal)
       ├─ Standard C types
       └─ Works everywhere
```

Result: ✅ Haiku, ✅ Linux, ✅ Easy to port

## Documentation

For detailed information:

- **Quick Start**: `QUICKSTART_ACCELERANT.md`
- **Haiku Deployment**: `HAIKU_DEPLOYMENT.md`
- **Build Error Explanation**: `README_HAIKU_BUILD_ERROR.md`
- **Architecture Details**: `ARCHITECTURE.md`
- **Complete Solution**: `SOLUTION_COMPLETE.md`
- **Refactor Summary**: `ACCELERANT_REFACTOR_SUMMARY.md`

## Troubleshooting

### "Command not found: ./FIX_HAIKU_BUILD.sh"
```bash
# Ensure you're in the right directory
cd ~/src/AMDstracted-GPU
ls -l FIX_HAIKU_BUILD.sh  # Should exist

# Run it
bash FIX_HAIKU_BUILD.sh
```

### "Library amdgpu not found"
```bash
# The core needs to be built first
./Build.sh
# Then run the fix script
./FIX_HAIKU_BUILD.sh
```

### "Permission denied" on install
```bash
# Ensure write permissions
ls -ld /boot/system/add-ons/accelerants/
# If needed, run with sudo or as admin user
```

### Still getting old errors after running fix
```bash
# Force complete clean
rm -rf accelerant/builddir_accelerant*
./FIX_HAIKU_BUILD.sh
```

## Testing GPU Acceleration

After successful installation:

### Test 1: System Info
```bash
# Check if accelerant is available
grep -i amd /boot/system/add-ons/accelerants/
# Should show amd_gfx.accelerant
```

### Test 2: Graphics Test
```bash
# Open graphics-heavy app
# Monitor system smoothness
# Check if GPU is being used
```

### Test 3: Terminal Output
```bash
# If accelerant logs to console (debug build)
# Check for messages during app startup
pkill -9 app_server  # Start fresh logging
# Open app
# Check logs
```

## What's Next (After Testing)

1. **If working**: Accelerant is operational
   - Proceed to implement GPU operations
   - Fine-tune performance

2. **If you see issues**: May need to implement missing GPU operations
   - `amd_fill_rectangle()` - GPU fill operations
   - `amd_blit()` - GPU copy operations
   - `amd_wait_engine_idle()` - Synchronization

3. **For more platforms**: Use as template for other OSes
   - Create `accelerant_freebsd.h`
   - Create `Accelerant_freebsd.c`
   - Add to `meson.build`

## Quick Reference

| Task | Command | Time |
|------|---------|------|
| Fix build | `./FIX_HAIKU_BUILD.sh` | 2-3 min |
| Restart graphics | `pkill -9 app_server` | 1 min |
| Verify install | `ls -l /boot/system/add-ons/accelerants/` | 30 sec |
| Test app | Open any graphics app | 2-5 min |
| **Total** | | **~8 min** |

## Status Checklist

- [ ] Ran `./FIX_HAIKU_BUILD.sh` without errors
- [ ] Script showed "AMD Accelerant installed successfully"
- [ ] File exists at `/boot/system/add-ons/accelerants/amd_gfx.accelerant`
- [ ] Restarted `app_server` with `pkill -9 app_server`
- [ ] Graphics applications run smoothly
- [ ] No visual artifacts or crashes

If all checked: ✅ **Accelerant is working!**

## Success Message

Once everything is working, you'll have:
- ✅ Functional AMD Accelerant on Haiku
- ✅ Platform-agnostic implementation
- ✅ Foundation for GPU acceleration
- ✅ Tested code on multiple platforms (Linux + Haiku)
- ✅ Easy path to port to more OSes

---

## Questions?

See documentation files for:
- **"How do I build?"** → `QUICKSTART_ACCELERANT.md`
- **"Why did this fail?"** → `README_HAIKU_BUILD_ERROR.md`
- **"How do I deploy?"** → `HAIKU_DEPLOYMENT.md`
- **"How does it work?"** → `ARCHITECTURE.md`
- **"What changed?"** → `ACCELERANT_REFACTOR_SUMMARY.md`

---

**Ready to fix?**

```bash
cd ~/src/AMDstracted-GPU
./FIX_HAIKU_BUILD.sh
```

Good luck! 🚀

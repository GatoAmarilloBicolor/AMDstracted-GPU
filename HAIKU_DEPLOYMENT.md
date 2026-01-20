# AMD Accelerant - Haiku OS Deployment Guide

## Current Status

You're running on **Haiku OS** with the refactored AMD Accelerant. The previous errors are from an outdated build directory with old source files.

## Quick Fix (5 minutes)

```bash
cd ~/src/AMDstracted-GPU
./FIX_HAIKU_BUILD.sh
```

This script will:
1. Clean outdated build directories
2. Verify all new headers are present
3. Fresh rebuild with Meson
4. Install to `/boot/system/add-ons/accelerants/`
5. Prepare system for testing

---

## Manual Steps (if script fails)

### Step 1: Clean Build Directory
```bash
cd ~/src/AMDstracted-GPU/accelerant
rm -rf builddir_accelerant builddir_accelerant_fixed
```

### Step 2: Fresh Meson Setup
```bash
meson setup builddir_accelerant \
    -Dprefix=~/src/AMDstracted-GPU/install.x86_64 \
    --buildtype=debugoptimized
```

Expected output:
```
Library be found: YES
Library device found: YES
Library amdgpu found: YES
Message: Found libamdgpu
Build targets in project: 2
```

### Step 3: Build
```bash
cd builddir_accelerant
ninja -j4
```

Expected: Compiles `Accelerant_v2.c` without errors

### Step 4: Install
```bash
ninja install
```

Expected location:
```
/boot/system/add-ons/accelerants/amd_gfx.accelerant
```

---

## Verify Installation

```bash
# Check file exists
ls -l /boot/system/add-ons/accelerants/amd_gfx.accelerant

# Check permissions
file /boot/system/add-ons/accelerants/amd_gfx.accelerant
```

Expected output:
```
ELF 64-bit LSB shared object, x86-64, ...
```

---

## Restart Graphics System

### Option 1: Restart app_server (safer)
```bash
pkill -9 app_server
# Haiku will automatically restart it
# Screen may blink
```

### Option 2: Full System Reboot (most reliable)
```bash
shutdown -r now
```

---

## Testing

### Test 1: Graphics System Status
```bash
# Check if accelerant loaded
deskbar → Team Monitor → Graphics (if available)
```

### Test 2: Run Graphics-Intensive App
```bash
# Open any graphics app or 3D benchmark
# Check if it runs smoothly
```

### Test 3: Monitor GPU Usage
```bash
# In another Terminal:
watch -n 1 'cat /dev/accelerant'
# (if system provides GPU stats)
```

---

## Troubleshooting

### "accelerant_api.h: No such file"
Problem: Header files not present

Solution:
```bash
# Verify headers exist
ls -l ~/src/AMDstracted-GPU/accelerant/include/
# Should show:
#   accelerant_api.h
#   accelerant_haiku.h

# If missing, pull latest from git:
cd ~/src/AMDstracted-GPU
git pull origin main
```

### "B_CONNECTION_REFUSED undeclared"
Problem: Using old Accelerant.c instead of Accelerant_v2.c

Solution:
```bash
# Clean and rebuild with provided script
./FIX_HAIKU_BUILD.sh
```

### "Cannot find libamdgpu"
Problem: Core AMDGPU_Abstracted not built

Solution:
```bash
cd ~/src/AMDstracted-GPU
./Build.sh  # Builds core libraries
./FIX_HAIKU_BUILD.sh  # Then builds accelerant
```

### Meson "not in list of build targets"
Problem: Corrupted build cache

Solution:
```bash
rm -rf accelerant/builddir_accelerant
meson setup accelerant/builddir_accelerant
cd accelerant/builddir_accelerant
ninja
ninja install
```

---

## Architecture Reminder

The refactored accelerant uses **3 layers**:

```
Haiku Graphics System
    ↓
[Haiku Adapter Layer] ← accelerant_haiku.h
    ↓
[Platform-Agnostic Core] ← Accelerant_v2.c (the NEW file)
    ↓
AMDGPU_Abstracted RMAPI
```

**Old files (deprecated, cause errors):**
- `Accelerant.c` - DON'T USE
- `HailuAMDInterface.c` - DON'T USE

**New files (use these):**
- `Accelerant_v2.c` - ✓ USE THIS
- `accelerant_api.h` - ✓ USE THIS
- `accelerant_haiku.h` - ✓ USE THIS

---

## Key Files

```
~/src/AMDstracted-GPU/
├── accelerant/
│   ├── include/
│   │   ├── accelerant_api.h       ← Platform-agnostic types
│   │   └── accelerant_haiku.h     ← Haiku adapter
│   ├── src/
│   │   ├── Accelerant_v2.c        ← NEW - use this
│   │   ├── Accelerant.c           ← OLD - don't use
│   │   └── HailuAMDInterface.c    ← OLD - don't use
│   ├── meson.build                 ← Updated for V2
│   └── builddir_accelerant/        ← Build directory
├── install.x86_64/                 ← Installation prefix
├── FIX_HAIKU_BUILD.sh             ← Fix script
├── build_accelerant.sh             ← Generic build script
└── FIX_HAIKU_BUILD.sh             ← Haiku-specific fix
```

---

## Success Indicators

✓ Script ran without errors  
✓ Installation message shown  
✓ File at `/boot/system/add-ons/accelerants/amd_gfx.accelerant`  
✓ File type is ELF 64-bit shared object  
✓ app_server restarted successfully  
✓ Graphics applications run smoothly  

---

## Need Help?

### Check Build Logs
```bash
# From the build directory:
cat meson-logs/meson-log.txt | tail -100
```

### Verify Meson Configuration
```bash
cd accelerant/builddir_accelerant
meson introspect --all | grep -i accelerant
```

### Manual Test Compile
```bash
# Test if headers compile
cc -c -Iaccelerate/include accelerant/src/Accelerant_v2.c
# Should produce: Accelerant_v2.o (no errors)
```

---

## Next After Deployment

Once accelerant is working:

1. **Implement GPU Operations** (TODO items in Accelerant_v2.c)
   - Fill rectangle operations
   - Blit operations
   - Fence synchronization

2. **Implement Display Management**
   - Query available modes
   - Set display mode
   - Hardware cursor support

3. **Performance Optimization**
   - Profile hot paths
   - Optimize GPU submissions
   - Cache mode lists

---

## Documentation

- **Architecture Details**: `ARCHITECTURE.md`
- **Refactor Summary**: `ACCELERANT_REFACTOR_SUMMARY.md`
- **Solution Complete**: `SOLUTION_COMPLETE.md`
- **Quick Start**: `QUICKSTART_ACCELERANT.md`

---

## Status

📍 **Location**: Haiku OS  
🔧 **Version**: Accelerant_v2.c (refactored, platform-agnostic)  
🟡 **Status**: Ready for installation & testing  
🎯 **Next**: Run `./FIX_HAIKU_BUILD.sh`  

---

## Success!

Once this is complete, you'll have:
- ✅ Working AMD Accelerant on Haiku
- ✅ Platform-agnostic implementation
- ✅ Foundation for GPU acceleration
- ✅ Easy path to port to other OSes

Good luck! 🚀

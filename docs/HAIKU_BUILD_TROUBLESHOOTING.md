# Haiku Build Troubleshooting Guide

**For**: Building AMD Unified Driver on Haiku OS  
**Status**: Common issues and solutions

---

## Issue 1: "unknown type name 'display_mode'"

### Error Message
```
error: unknown type name 'display_mode'
  168 | int amdgpu_set_display_mode_hal(struct OBJGPU *adev, const display_mode *mode);
```

### Cause
`display_mode` type from `GraphicsDefs.h` not available when header is parsed.

### Solution
The unified driver includes `GraphicsDefs.h` in `config/config.h` (which is included before all other headers). This ensures the type is available.

**If you still get this error:**

1. **Check config.h includes GraphicsDefs.h**
   ```bash
   grep -n "GraphicsDefs.h" config/config.h
   ```

2. **Verify config.h is included first**
   ```bash
   # In Makefile or build script, ensure:
   -include config/config.h
   # comes BEFORE any other includes

3. **Clean and rebuild**
   ```bash
   rm -rf build_haiku build
   ./scripts/haiku_build.sh
   ```

---

## Issue 2: "clang: command not found"

### Error Message
```
./scripts/haiku_build.sh: line X: clang: command not found
ERROR: No suitable C compiler found
```

### Cause
C compiler not installed on Haiku.

### Solution
Install gcc or clang:

```bash
pkgman install gcc
# or
pkgman install clang
```

Verify installation:
```bash
gcc --version
# or
clang --version
```

---

## Issue 3: "Haiku headers not found"

### Error Message
```
ERROR: Haiku headers not found
Install with: pkgman install haiku_devel
```

### Cause
Haiku development headers not installed.

### Solution
Install development package:

```bash
pkgman install haiku_devel
```

Verify:
```bash
ls -d /boot/system/develop/headers
```

---

## Issue 4: "cannot find -ldl"

### Error Message
```
/usr/bin/ld: cannot find -ldl
```

### Cause
Dynamic linking library not available (unusual on Haiku, but can happen).

### Solution
On Haiku, dynamic linking is built-in. Try:

1. **Skip -ldl flag**
   ```bash
   # Edit haiku_build.sh, remove -ldl from gcc/clang commands
   ```

2. **Or install libc devel**
   ```bash
   pkgman install libc_devel
   ```

---

## Issue 5: Compilation Timeout or Hangs

### Cause
Limited resources on Haiku system, or build taking too long.

### Solution
1. **Use faster compiler (clang > gcc)**
   ```bash
   which clang && echo "Clang available"
   ```

2. **Reduce optimization level**
   ```bash
   # Edit haiku_build.sh, change -O2 to -O0 if needed
   ```

3. **Check available memory**
   ```bash
   free -h
   ```

---

## Issue 6: Tests Pass Locally but Fail on Haiku

### Cause
Environment differences between Linux and Haiku.

### Solution
1. **Check Haiku-specific code paths**
   ```bash
   grep -r "__HAIKU__" src/
   ```

2. **Verify environment variables are set**
   ```bash
   # On Haiku
   env | grep AMD_GPU
   ```

3. **Enable debug output**
   ```bash
   # In config/config.h
   #define AMD_DEBUG 1
   #define AMD_LOG_LEVEL 3
   ```

4. **Rebuild with debug**
   ```bash
   ./scripts/haiku_build.sh
   ./build_haiku/test_unified_driver
   ```

---

## Issue 7: Accelerant Integration Fails

### Cause
Library path or symbol issues.

### Solution
1. **Verify library is built**
   ```bash
   ls -lh build_haiku/libamd_unified_haiku.so
   ldd build_haiku/libamd_unified_haiku.so
   ```

2. **Check symbols are exported**
   ```bash
   nm build_haiku/libamd_unified_haiku.so | grep amd_device_probe
   ```

3. **Copy to system path**
   ```bash
   cp build_haiku/libamd_unified_haiku.so /boot/system/lib/
   ```

4. **Update accelerant to link correctly**
   ```bash
   gcc -o my_accelerant ... -L/boot/system/lib -lamd_unified_haiku
   ```

---

## Testing Verification

### Step 1: Build Successfully
```bash
./scripts/haiku_build.sh
# Should show:
# ✓ libamd_unified_haiku.so created
# ✓ test_unified_driver created
```

### Step 2: Run Tests
```bash
./build_haiku/test_unified_driver
# Should show:
# ✓ Device Detection
# ✓ Backend Selection
# ✓ Handler Assignment
# ... (7 tests total)
# Result: 7/7 PASSING
```

### Step 3: Verify Environment Setup
```bash
./build_haiku/test_unified_driver 2>&1 | grep -A5 "Haiku"
# Should show environment variables are being set
```

---

## Build Output Examples

### Successful Build
```
╔════════════════════════════════════════════════════════════════╗
║        AMD Unified GPU Driver - Haiku Build Script            ║
╚════════════════════════════════════════════════════════════════╝

✓ Compiler: clang
✓ Haiku headers found

[1/4] Compiling backend detection...
  ✓ amd_backend_detection.o
[2/4] Compiling device core...
  ✓ amd_device_core.o
[3/4] Compiling handlers...
  ✓ vliw_handler.o
  ✓ gcn_handler.o
  ✓ rdna_handler.o
[4/4] Compiling device database...
  ✓ amd_devices.o

Linking Library...
✓ libamd_unified_haiku.so created
-rwxr-xr-x 1 user group 22K build_haiku/libamd_unified_haiku.so

Building Test Suite...
✓ test_unified_driver created
-rwxr-xr-x 1 user group 31K build_haiku/test_unified_driver

Build Complete!
```

### Successful Test Run
```
=== AMD Unified Driver Test Suite ===

  ✓ Warrior (VLIW) detected
  ✓ Device probe successful
✓ Device Detection

  ✓ Warrior (VLIW) → Mesa
✓ Backend Selection

[Detecting backend support on this system]
[Backend Detection]
  Checking RADV (Vulkan)... ✗ not found
  Checking Mesa Gallium (OpenGL)... ✓ available

[Setting Environment Variables]
  Setting AMD_GPU_BACKEND=gallium
  Setting LIBGL_ALWAYS_INDIRECT=1
  Setting MESA_GL_VERSION_OVERRIDE=4.5
  Selected: Mesa Gallium
  Note: Auto-detected: Mesa Gallium (OpenGL)

✓ Device Initialization (Warrior with Mesa)

=== Test Summary ===
Passed: 7
Failed: 0
Total:  7
```

---

## Advanced Troubleshooting

### Enable Verbose Build
```bash
# Edit haiku_build.sh, change each gcc/clang call to:
$CC ... -v ...  # Add -v flag for verbose output
```

### Check Intermediate Files
```bash
ls -lh build_haiku/
# Should show .o files for each source
```

### Verify Include Paths
```bash
# Check Haiku headers are correct
ls /boot/system/develop/headers/graphics/
# Should include GraphicsDefs.h
```

### Test Individual Components
```bash
# Compile just device detection
gcc -D__HAIKU__ -std=c99 -include config/config.h \
    -Iconfig -Isrc/amd \
    -I/boot/system/develop/headers \
    -c src/amd/amd_backend_detection.c

# Should succeed without errors
```

---

## Getting Help

If you encounter issues:

1. **Check this document** - Most common issues are listed
2. **Read docs/HAIKU_INTEGRATION.md** - Integration guide
3. **Check git log** - See what was changed recently
4. **Run with debug output**
   ```bash
   # Rebuild with debug enabled
   # Set AMD_DEBUG=1 in config/config.h
   ```
5. **Check GitHub issues** - May have been reported

---

## File Locations

Main files involved in build:

```
config/config.h                    # OS detection, includes
src/amd/amd_device.h               # Core API
src/amd/amd_backend_detection.h/c  # Backend system
src/amd/handlers/                  # Handler implementations
scripts/haiku_build.sh             # This build script
```

---

## Quick Fixes Checklist

- [ ] GraphicsDefs.h included in config.h
- [ ] Compiler installed (gcc or clang)
- [ ] Haiku headers installed (haiku_devel)
- [ ] Build directory is clean
- [ ] All source files present
- [ ] Script has executable permissions
- [ ] No syntax errors in edited files

---

**Last Updated**: January 17, 2026  
**Status**: Complete troubleshooting guide ready

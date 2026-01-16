# 🎯 HAIKU TESTING GUIDE - OS-Agnostic AMD GPU Driver

**Status:** ✅ Ready for Haiku Testing  
**Branch:** main  
**Latest Commit:** a353b03 - Complete graceful fallback for cross-platform compilation

---

## 📋 QUICK START - ON HAIKU SYSTEM

### 1. Clone Repository
```bash
git clone https://github.com/GatoAmarilloBicolor/AMDstracted-GPU.git
cd AMDstracted-GPU
```

### 2. Build for Haiku
```bash
# Simple build
make OS=haiku clean && make OS=haiku

# Or using the build script
chmod +x build_agnosis.sh
./build_agnosis.sh haiku
```

### 3. Run Tests
```bash
# Start RMAPI server
./rmapi_server &

# Run client test
./rmapi_client_demo

# Kill server
pkill rmapi_server
```

### 4. Expected Output

**Server should show:**
```
[LOG] HAL: Starting the GPU City (HIT Edition)
[LOG] HAL: Identified GPU: Radeon HD 7290 (Wrestler)
[LOG] HAL: Loading Wrestler APU specialists...
[LOG] GMC v10: [Early] Checking if memory controller is alive...
[LOG] GMC v10: [SW Init] Setting up page tables...
[LOG] GMC v10: [HW Init] Programming memory controller...
[LOG] GFX v10: [Early] Checking if graphics engine is alive...
...
Yo! RMAPI Server is live on /tmp/amdgpu_hit.sock
```

**Client should show:**
```
🌀 HIT Client: Connecting to GPU Subway...
✅ Connected to the Driver Brain!
📡 GPU Identity Confirmed!
📍 Name:   Radeon HD 7290 (Wrestler)
📍 VRAM:   512 MB
📍 Clock:  400 MHz
👋 Closing connection.
```

---

## 🔧 COMPILATION DETAILS

### What Gets Built

| File | Purpose |
|------|---------|
| `libamdgpu.so` | Core GPU driver library |
| `rmapi_server` | RMAPI daemon (IPC server) |
| `rmapi_client_demo` | Test client application |
| `amdgpu_hit` | Haiku addon (if C++ available) |
| `amdgpu_hit.accelerant` | Haiku accelerant (if C++ available) |

### Compiler Flags

When building with `OS=haiku`, the Makefile automatically:
```makefile
CFLAGS += -D__HAIKU__                    # Define Haiku flag
LDFLAGS += -lroot -lnetwork              # Link Haiku libraries
```

This enables real Haiku APIs:
- `<OS.h>` - Haiku OS primitives
- `<device/device_manager.h>` - Device enumeration
- `<drivers/pci/pci.h>` - PCI scanning
- Haiku semaphores and threads

---

## 🧪 TESTING CHECKLIST

### ✅ Compilation
- [ ] `make OS=haiku clean` completes
- [ ] `make OS=haiku` compiles without errors
- [ ] `libamdgpu.so` is created
- [ ] `rmapi_server` is created

### ✅ Runtime
- [ ] `./rmapi_server` starts without crashing
- [ ] `./rmapi_client_demo` connects successfully
- [ ] GPU info is displayed correctly
- [ ] Server shuts down cleanly

### ✅ Features
- [ ] Device enumeration works (detects AMD GPU)
- [ ] Memory allocation succeeds
- [ ] MMIO access works (no segfaults)
- [ ] IPC communication is bidirectional
- [ ] Logging output is visible

### ✅ Integration
- [ ] Haiku accelerant loads (if available)
- [ ] Device manager integration works
- [ ] Display detection functions (if DCE implemented)

---

## 📊 ARCHITECTURE ON HAIKU

```
Application Layer
    ↓
HAL (gmc_v10, gfx_v10, rmapi) ← NO platform-specific includes
    ↓
Abstraction Layer (os_abstract_*.h)
    ↓
Haiku Implementation (os_primitives_haiku.c with #if HAIKU_NATIVE)
    ↓
Haiku OS APIs
    - device_manager (PCI enumeration)
    - snooze() (delays)
    - semaphores (synchronization)
    - dprintf() (logging)
```

---

## 🔍 DEBUGGING

### Enable Verbose Logging
```bash
# Modify src/amd/rmapi.c or kernel-amd/os-primitives/haiku/os_primitives_haiku.c
// Change log level in logger initialization
os_logger_config_t cfg = {
    .target = OS_LOG_TARGET_STDERR,
    .min_level = OS_LOG_DEBUG,    // <- TRACE for more verbosity
};
os_logger_init(&cfg);
```

### Check Compilation Flags
```bash
make OS=haiku 2>&1 | grep "^cc.*-D"
# Should show: cc -D__HAIKU__ ...
```

### Verify Device Detection
```bash
./rmapi_server 2>&1 | grep -i "pci\|device\|gpu"
```

### Monitor System Resources
```bash
# In another terminal while server runs
ps aux | grep rmapi
# Should show rmapi_server running

# Check memory usage
sysinfo | grep Memory
```

---

## 🚨 POTENTIAL ISSUES & SOLUTIONS

### Issue: "fatal error: OS.h: No such file or directory"
**Solution:** This is expected on non-Haiku systems. On Haiku with SDK:
```bash
# Ensure Haiku headers are installed
listimage | grep libroot    # Should see Haiku libraries
```

### Issue: "undefined reference to -lroot"
**Solution:** Haiku system libraries not available (expected on Linux). On Haiku:
```bash
# Install development headers if needed
pkgman install haiku_devel
```

### Issue: Server crashes with segfault
**Solution:** Check stack trace:
```bash
gdb ./rmapi_server
(gdb) run
(gdb) bt    # backtrace
```

### Issue: GPU not detected
**Solution:** Verify device detection:
```bash
./rmapi_server 2>&1 | head -20
# Should show device enumeration attempts
```

---

## 📦 WHAT'S IMPLEMENTED

### ✅ OS-Primitives (Haiku)
- Device manager integration for PCI scanning
- Haiku semaphores for synchronization
- Haiku thread spawning
- Haiku delays (snooze)
- dprintf logging
- Graceful fallback when headers unavailable

### ✅ Abstraction Layers
- **IPC:** Unix sockets (POSIX - works on Haiku)
- **Logging:** Multi-target with Haiku support
- **Threading:** Haiku native with fallback
- **MMIO:** Safe register access with bounds checking

### ✅ HAL Layer
- GMC v10 (Memory Controller) - Real register programming
- GFX v10 (Graphics Engine) - Ring buffer setup
- RMAPI (High-level API) - User space interface

### ✅ Testing Infrastructure
- Client/Server IPC over sockets
- GPU info queries
- Memory management testing
- Build system for multiple platforms

---

## 🎯 NEXT STEPS AFTER TESTING

### If Compilation Succeeds
1. Test basic functionality (run client)
2. Verify logging output
3. Check device detection
4. Test memory allocation

### If Integration Works
1. Implement real DCE (Display Controller Engine)
2. Add Haiku accelerant hooks
3. Implement actual framebuffer mapping
4. Add interrupt handling

### For Full GPU Support
1. Implement RADV winsys backend
2. Add Vulkan device support
3. Integrate with Zink
4. Test OpenGL via Zink

---

## 📞 COMMIT HISTORY (HAIKU-RELEVANT)

```
a353b03 - fix: Complete graceful fallback for cross-platform compilation
2dcbbdf - fix: Add OS-specific defines to CFLAGS
e70550a - docs: Add implementation summary
5c74a69 - feat: Add build_agnosis.sh script
2366d19 - fix: Make os_primitives conditional
9c1a040 - feat: Complete OS-agnostic abstraction layer
```

Each commit is self-contained and can be tested independently.

---

## 📝 EXPECTED BEHAVIOR ON HAIKU

### With Real Haiku System (Headers Available)
✅ Compiles with real Haiku APIs  
✅ Uses device_manager for PCI enumeration  
✅ Uses Haiku semaphores and threads  
✅ Full hardware support  

### Without Haiku Headers (Cross-compile)
✅ Still compiles successfully  
✅ Uses stubs instead of real APIs  
✅ Graceful fallback to simulation  
✅ Same binaries work on Haiku (no headers needed)  

---

## 🏆 SUCCESS CRITERIA

**Basic Success:**
- [ ] Compilation completes without errors
- [ ] rmapi_server starts
- [ ] rmapi_client_demo connects

**Full Success:**
- [ ] GPU information displayed correctly
- [ ] No segmentation faults
- [ ] Memory operations work
- [ ] Logging shows proper initialization

**Advanced Success:**
- [ ] Device manager integration confirmed
- [ ] PCI device enumeration works
- [ ] Real GPU info (not simulation)
- [ ] Haiku accelerant loads

---

## 📚 DOCUMENTATION

All relevant documentation:
- `AGNOSIS_IMPLEMENTATION.md` - Complete implementation guide
- `OS_ABSTRACTION_BEST_PRACTICES.md` - Architecture patterns
- `AGNOSIS_COMPLETION_SUMMARY.md` - Overview of agnosis layer
- `build_agnosis.sh` - Multi-platform build script

---

**Ready to test on Haiku! 🚀**

*Latest changes: Complete graceful fallback with #if HAIKU_NATIVE guards*  
*All code is agnóstic and ready for production testing*

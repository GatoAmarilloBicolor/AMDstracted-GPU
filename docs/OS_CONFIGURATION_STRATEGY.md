# OS Configuration Strategy: Linux vs Haiku

**Goal**: Adapt the unified driver to different operating systems while maintaining code reuse and hardware compatibility.

---

## Problem Statement

The driver needs to work on multiple platforms (Linux, Haiku, FreeBSD) but each has different graphics stack requirements:

- **Linux**: Mature graphics ecosystem, users expect standard environment variables
- **Haiku**: Simpler, self-contained accelerant system, benefits from automatic setup
- **FreeBSD**: drm-kmod integration, different driver model

Previously, we would blindly set environment variables on all platforms, potentially affecting user settings.

---

## Solution: OS-Aware Configuration

```
Detect GPU Hardware
        ↓
Detect Available Libraries
        ↓
Select Best Backend
        ↓
Check Operating System ← NEW
        ├─ Linux: Suggest (print export commands)
        ├─ Haiku: Auto-configure (setenv)
        └─ FreeBSD: (future)
        ↓
Ready for Graphics Stack
```

---

## Implementation

### Linux Behavior

**What happens:**
1. Detect GPU capabilities (RADV support, etc.)
2. Check which graphics libraries are installed
3. Auto-select best backend
4. **Print suggestions** as bash export commands

**User sees:**
```
[Suggested Environment Variables]
  (Not setting on Linux - configure manually if needed)

  export AMD_GPU_BACKEND=radv
  export VK_DRIVER_FILES=/etc/vulkan/icd.d/amd_icd.json
```

**Advantages:**
- ✓ Doesn't modify user's environment
- ✓ User can review recommendations
- ✓ Easy to copy-paste if wanted
- ✓ Non-intrusive, safe for testing

**Code:**
```c
#ifdef __linux__
    // On Linux: only print, don't call setenv()
    printf("  export AMD_GPU_BACKEND=%s\n", backend_choice);
#endif
```

### Haiku Behavior

**What happens:**
1. Detect GPU capabilities (likely Mesa only for legacy GPUs)
2. Check which graphics libraries are installed
3. Auto-select best backend
4. **Automatically set environment variables**

**System sees:**
```
[Setting Environment Variables]
  Setting AMD_GPU_BACKEND=gallium
  Setting LIBGL_ALWAYS_INDIRECT=1
  Setting MESA_GL_VERSION_OVERRIDE=4.5
```

**Advantages:**
- ✓ Zero user configuration needed
- ✓ Graphics stack is ready to use
- ✓ Simplifies accelerant integration
- ✓ Ensures compatibility

**Code:**
```c
#ifdef __HAIKU__
    // On Haiku: automatically configure
    setenv("AMD_GPU_BACKEND", backend_choice, 1);
    setenv("LIBGL_ALWAYS_INDIRECT", "1", 1);
    // ... etc
#endif
```

---

## Architecture in Code

### Detection Phase

```c
amd_backend_support_t support;
amd_detect_backend_support(&support);
// ↓ Now support struct contains:
//   - system_has_radv, system_has_mesa, etc.
//   - hw_supports_vulkan, hw_supports_opengl
//   - env_override if user set AMD_GPU_BACKEND
```

### Configuration Phase

```c
amd_setup_backend_env(&support);
// ↓ Internally:
//   #ifdef __HAIKU__
//     setenv(...)    // Auto-configure on Haiku
//   #else
//     printf(...)    // Only suggest on Linux
//   #endif
```

### Information Phase

```c
amd_print_backend_support(&support);
// ↓ Shows what was detected and what was selected
```

---

## Hardware Compatibility Examples

### Scenario 1: Lucienne (RDNA2) on Linux

```
GPU: Lucienne (RDNA2) → Supports RADV natively
Detect: RADV available? Yes

[Suggested Environment Variables]
  export AMD_GPU_BACKEND=radv
  export VK_DRIVER_FILES=/etc/vulkan/icd.d/amd_icd.json

User: Can copy-paste if desired
```

### Scenario 2: Warrior (VLIW) on Haiku

```
GPU: Warrior (VLIW) → No native RADV support
Detect: RADV available? No → Use Mesa

[Setting Environment Variables]
  Setting AMD_GPU_BACKEND=gallium
  Setting LIBGL_ALWAYS_INDIRECT=1
  Setting MESA_GL_VERSION_OVERRIDE=4.5

Haiku: Automatically ready, no user action
```

### Scenario 3: Polaris (GCN3) on Linux with RADV unavailable

```
GPU: Polaris → Hardware supports RADV, but not installed
Detect: RADV available? No → Fallback to Mesa

[Suggested Environment Variables]
  export AMD_GPU_BACKEND=gallium
  export LIBGL_ALWAYS_INDIRECT=1
  export MESA_GL_VERSION_OVERRIDE=4.5

User: Mesa is better than nothing, not optimal but works
```

---

## User Workflows

### Linux: Manual Configuration

```bash
# Run accelerant/driver
/path/to/accelerant &

# Get suggested exports
export AMD_GPU_BACKEND=radv
export VK_DRIVER_FILES=/etc/vulkan/icd.d/amd_icd.json

# Run application
glxgears
```

### Linux: Automatic (with script)

```bash
#!/bin/bash
eval "$(./accelerant 2>&1 | grep '^  export')"
glxgears
```

### Haiku: Zero Configuration

```bash
# Run accelerant
/system/add-ons/accelerants/amd_unified

# Immediately ready - all env vars are set
# No user action needed
glxgears
```

---

## Environment Variables Set

### RADV (Vulkan)
```bash
AMD_GPU_BACKEND=radv
VK_DRIVER_FILES=/etc/vulkan/icd.d/amd_icd.json
```

### Zink (OpenGL via Vulkan)
```bash
AMD_GPU_BACKEND=zink
LIBGL_ALWAYS_INDIRECT=1
MESA_GL_VERSION_OVERRIDE=4.5
VK_DRIVER_FILES=/etc/vulkan/icd.d/amd_icd.json
```

### Mesa Gallium (Native OpenGL)
```bash
AMD_GPU_BACKEND=gallium
LIBGL_ALWAYS_INDIRECT=1
MESA_GL_VERSION_OVERRIDE=4.5
```

### LLVMPipe (Software)
```bash
AMD_GPU_BACKEND=llvmpipe
LIBGL_ALWAYS_INDIRECT=1
MESA_GL_VERSION_OVERRIDE=4.5
```

---

## User Override Priority

1. **Environment variable** (highest)
   ```bash
   # User explicitly sets
   export AMD_GPU_BACKEND=mesa
   ```

2. **Auto-detected best** (default)
   - Hardware supports it
   - System has the library
   - Not overridden by user

3. **Fallback** (last resort)
   - If preferred not available
   - Use next best option

---

## Testing Strategy

### Linux Testing
```bash
make -f Makefile.unified run-tests
# Tests will show suggestions, not set vars
# Can verify with: env | grep AMD_GPU
```

### Haiku Testing
```bash
# Transfer and run
scp build_haiku/test_unified_driver user@haiku:
ssh user@haiku ./test_unified_driver
# Will show "Setting" instead of "Suggested"
# Verify with: env | grep AMD_GPU
```

---

## Benefits of This Approach

### For Linux Users
- ✓ Safe testing without modifying environment
- ✓ Can review recommendations
- ✓ Maintains control over their system
- ✓ Can override if they want different backend
- ✓ Easy to integrate into scripts

### For Haiku System
- ✓ Zero configuration needed
- ✓ Graphics stack is ready
- ✓ Accelerant just works
- ✓ Good user experience
- ✓ No manual setup errors

### For Developers
- ✓ Single codebase for both platforms
- ✓ Conditional compilation is clean
- ✓ Easy to test different configurations
- ✓ Easy to add new OS support

---

## Future Platforms

### FreeBSD (drm-kmod)

```c
#ifdef __FreeBSD__
    // Could use different strategy
    // Maybe system-wide config file?
    // Or defer to drm-kmod bridge?
#endif
```

### Android/ChromeOS (if needed)

```c
#ifdef __ANDROID__
    // Android has different graphics stack
    // Would need separate strategy
#endif
```

---

## Code Locations

**Detection & Configuration:**
```
src/amd/
├── amd_backend_detection.h      (API)
└── amd_backend_detection.c      (Implementation with OS checks)
```

**Lifecycle Integration:**
```
src/amd/
└── amd_device_core.c            (Calls backend setup during init)
```

**Documentation:**
```
docs/
├── BACKEND_DETECTION.md         (Detailed backend info)
└── OS_CONFIGURATION_STRATEGY.md (This file)
```

---

## Conditional Compilation Markers

The driver uses standard compiler markers:

```c
#ifdef __HAIKU__
    // Haiku-specific code
#elif defined(__linux__)
    // Linux-specific code
#elif defined(__FreeBSD__)
    // FreeBSD-specific code
#else
    // Generic fallback
#endif
```

These are:
- Defined by the compiler automatically
- No external configuration needed
- Easy to test with `-D__HAIKU__` flag

---

## Configuration Summary

| Aspect | Linux | Haiku | FreeBSD |
|--------|-------|-------|---------|
| **Detection** | ✓ Yes | ✓ Yes | ✓ Yes (future) |
| **Env Setup** | Suggest only | Auto-set | TBD |
| **User Control** | Manual | Automatic | TBD |
| **System Impact** | None | Minimal | TBD |
| **Compatibility** | High | High | High |

---

## Conclusion

This OS-aware strategy provides:
- **Safe testing** on Linux (non-intrusive)
- **Seamless setup** on Haiku (automatic)
- **Single codebase** for all platforms
- **Easy extensibility** for future OS support

The driver detects what hardware can do, what system has available, and configures appropriately for each OS.

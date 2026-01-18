# Backend Detection System

**Purpose**: Detect available graphics backends at runtime and select the best one for the detected GPU hardware.

---

## Overview

The unified driver now includes intelligent backend detection that:

1. **Detects GPU Capabilities** - What does the hardware support?
2. **Detects System Libraries** - What's actually installed?
3. **Checks Environment Overrides** - User preferences via env vars
4. **Selects Best Backend** - RADV → Zink → Mesa → Software
5. **Sets Environment Variables** - Configures the graphics stack

---

## Architecture

```
GPU Hardware Detected
        ↓
[Backend Detection System]
├─ Hardware Capabilities
│  ├─ Vulkan support? (RDNA2+)
│  ├─ OpenGL support? (all)
│  └─ Compute support? (GCN+)
│
├─ System Libraries
│  ├─ libvulkan.so? (RADV)
│  ├─ libzink.so? (GL→Vulkan)
│  ├─ libGL.so? (Mesa Gallium)
│  └─ libllvmpipe.so? (Software)
│
├─ Environment Override
│  └─ AMD_GPU_BACKEND = {auto|radv|zink|gallium|llvmpipe}
│
└─ Selection Logic
   └─ Choose best available option
        ↓
[Environment Variables Set]
├─ AMD_GPU_BACKEND
├─ LIBGL_ALWAYS_INDIRECT
├─ MESA_GL_VERSION_OVERRIDE
└─ VK_DRIVER_FILES
        ↓
Ready for Graphics Applications
```

---

## Backend Selection Hierarchy

### Automatic Selection (default)

```
Modern GPU (RDNA2+) with Vulkan support?
├─ YES → RADV (Vulkan) ✓
└─ NO  → Zink available?
         ├─ YES → Zink (OpenGL via Vulkan) ✓
         └─ NO  → Mesa available?
                  ├─ YES → Mesa Gallium (OpenGL) ✓
                  └─ NO  → LLVMPipe (software) ✓
```

### With Environment Override

Set `AMD_GPU_BACKEND` to force a specific backend:

```bash
# Use RADV (Vulkan) if available
export AMD_GPU_BACKEND=radv

# Use Zink (OpenGL via Vulkan)
export AMD_GPU_BACKEND=zink

# Use Mesa Gallium (native OpenGL)
export AMD_GPU_BACKEND=gallium

# Use software rendering
export AMD_GPU_BACKEND=llvmpipe

# Auto-detect (default)
export AMD_GPU_BACKEND=auto
# or unset it
unset AMD_GPU_BACKEND
```

---

## Implementation Details

### Library Detection

The driver uses dynamic library loading to check availability:

```c
/* Check if RADV is available */
dlopen("libvulkan.so.1")    // Primary
dlopen("libvulkan.so")      // Fallback
dlopen("libradv.so")        // Direct

/* Check if Zink is available */
dlopen("libzink.so")

/* Check if Mesa is available */
dlopen("libGL.so.1")
dlopen("libGL.so")
dlopen("libgallium.so")

/* Check if software rendering is available */
dlopen("libllvmpipe.so")
```

No library is permanently loaded - we just check if dlopen succeeds.

### Hardware Capability Inference

Based on GPU generation:

```
VLIW (legacy)
├─ Hardware Vulkan: ✗
├─ Hardware OpenGL: ✓
└─ Default: Mesa Gallium

GCN1-5 (mid-range)
├─ Hardware Vulkan: ✓ (RADV)
├─ Hardware OpenGL: ✓ (Mesa)
└─ Default: RADV (if available)

RDNA2+ (modern)
├─ Hardware Vulkan: ✓ (RADV)
├─ Hardware OpenGL: ✓ (Zink or Mesa)
├─ Ray Tracing: ✓
└─ Default: RADV (if available)
```

---

## Usage Examples

### Example 1: Auto-detection (Default)

```c
#include "amd_backend_detection.h"

amd_backend_support_t support;
amd_detect_backend_support(&support);  // Detect what's available
amd_setup_backend_env(&support);       // Set env vars
amd_print_backend_support(&support);   // Show what was chosen

// Now fork/exec graphics application
// It will use the configured backend
```

**Output for Warrior (VLIW) on Haiku:**
```
[Backend Detection]
  Checking RADV (Vulkan)... ✗ not found
  Checking Zink (GL→Vulkan)... ✗ not found
  Checking Mesa Gallium (OpenGL)... ✓ available
  Checking LLVMPipe (software)... ✗ not found

[Setting Environment Variables]
  Setting AMD_GPU_BACKEND=gallium
  Setting LIBGL_ALWAYS_INDIRECT=1 (OpenGL mode)
  Setting MESA_GL_VERSION_OVERRIDE (compatibility)
  Selected: Mesa Gallium
  Note: Auto-detected: Mesa Gallium (OpenGL)
```

### Example 2: User Override on Lucienne

```bash
# Force Zink even though RADV is available
export AMD_GPU_BACKEND=zink
./my_app

# Force Mesa even though RADV is available
export AMD_GPU_BACKEND=gallium
./my_app
```

The driver will detect user preference and use it if available.

### Example 3: Headless System

On a system with no display libraries:

```c
amd_detect_backend_support(&support);  // Will find no libraries
amd_setup_backend_env(&support);       // Automatically selects LLVMPipe
```

Useful for compute-only workloads or containers.

---

## Environment Variables Set

When a backend is selected, the driver sets:

### For RADV (Vulkan)
```bash
AMD_GPU_BACKEND=radv
VK_DRIVER_FILES=/etc/vulkan/icd.d/amd_icd.json
```

### For Zink (OpenGL via Vulkan)
```bash
AMD_GPU_BACKEND=zink
LIBGL_ALWAYS_INDIRECT=1
MESA_GL_VERSION_OVERRIDE=4.5
VK_DRIVER_FILES=/etc/vulkan/icd.d/amd_icd.json
```

### For Mesa Gallium (Native OpenGL)
```bash
AMD_GPU_BACKEND=gallium
LIBGL_ALWAYS_INDIRECT=1
MESA_GL_VERSION_OVERRIDE=4.5
```

### For LLVMPipe (Software)
```bash
AMD_GPU_BACKEND=llvmpipe
LIBGL_ALWAYS_INDIRECT=1
MESA_GL_VERSION_OVERRIDE=4.5
```

---

## Hardware Compatibility Matrix

| GPU | Gen | HW Vulkan | HW OpenGL | Best Backend | Fallback |
|-----|-----|-----------|-----------|--------------|----------|
| Warrior | VLIW | ✗ | ✓ | Mesa | LLVMPipe |
| Tahiti | GCN1 | ✓* | ✓ | RADV | Mesa |
| Hawaii | GCN2 | ✓* | ✓ | RADV | Mesa |
| Fiji | GCN3 | ✓* | ✓ | RADV | Mesa |
| Vega | GCN4 | ✓ | ✓ | RADV | Mesa |
| Navi10 | GCN5 | ✓ | ✓ | RADV | Mesa |
| Lucienne | RDNA2 | ✓ | ✓ | RADV | Zink |
| RX 6800 | RDNA2 | ✓ | ✓ | RADV | Zink |
| RX 7900 | RDNA3 | ✓ | ✓ | RADV | Zink |

* = Supported via RADV but may have limitations

---

## Haiku Integration

For Haiku systems, the backend detection adapts:

```c
#ifdef __HAIKU__
/* Haiku doesn't have traditional RADV/Zink */
/* Detection still works but prefers Mesa/Gallium */
/* LLVMPipe available as fallback */
#endif
```

### On Haiku with Warrior:
```
System: Haiku OS
GPU: Warrior (VLIW)
Detected Backends:
  ✓ Mesa (custom Haiku port)
  ✗ RADV (Vulkan not standard)
  ✗ Zink
  ✗ LLVMPipe
Selected: Mesa Gallium
```

---

## Troubleshooting

### "No suitable backend found"

Your system has no graphics libraries installed. Solutions:

1. **Install Mesa**
   ```bash
   # Linux
   sudo apt-get install libgl1-mesa-glx
   
   # Haiku
   pkgman install mesa_devel
   ```

2. **Install Vulkan**
   ```bash
   # Linux
   sudo apt-get install libvulkan1
   ```

3. **Use software rendering**
   ```bash
   export AMD_GPU_BACKEND=llvmpipe
   ```

### "RADV requested but not available"

RADV library not found. Check:

```bash
# Find Vulkan
ldconfig -p | grep vulkan
find /usr -name "*vulkan*" 2>/dev/null

# Or force Mesa
export AMD_GPU_BACKEND=gallium
```

### "Zink not found"

Zink requires both RADV and Mesa. Install both:

```bash
# Ubuntu/Debian
sudo apt-get install libvulkan1 libgl1-mesa-glx

# Fedora
sudo dnf install vulkan-loader mesa-libGL

# Or skip Zink
export AMD_GPU_BACKEND=radv
```

---

## Performance Notes

**Benchmark Priority** (fastest to slowest):

1. RADV (native Vulkan) - Direct GPU access
2. Zink (GL→Vulkan) - Thin translation layer
3. Mesa Gallium (native OpenGL) - Direct GPU access
4. LLVMPipe (software) - CPU-based rendering

For maximum performance:
- Use RADV on modern GPUs (RDNA2+)
- Use Mesa on legacy (VLIW, GCN)
- Avoid LLVMPipe unless no alternative

---

## Code Reference

### Detection Function
```c
/* Check what's available on this system */
int amd_detect_backend_support(amd_backend_support_t *support);
```

### Environment Setup Function
```c
/* Configure system based on detection */
int amd_setup_backend_env(amd_backend_support_t *support);
```

### Library Check Function
```c
/* Check if a specific library is available */
bool amd_check_library_available(const char *libname);
```

### Environment Override
```c
/* Check AMD_GPU_BACKEND env var */
amd_env_backend_t amd_get_env_override(void);
```

---

## Future Enhancements

1. **Per-GPU Optimization**
   - Store benchmark data for different GPUs
   - Recommend best backend for each model

2. **GPU-Specific Tuning**
   - VLIW-specific Mesa settings
   - GCN async compute setup
   - RDNA ray tracing configuration

3. **Dynamic Switching**
   - Switch backends without restart
   - Fallback on error

4. **Plugin System**
   - Custom backend plugins
   - Third-party integration

---

## File Locations

```
src/amd/
├── amd_backend_detection.h      (API definitions)
└── amd_backend_detection.c      (Implementation)

docs/
└── BACKEND_DETECTION.md         (This file)
```

---

## Related Documentation

- [UNIFIED_DRIVER_BUILD.md](UNIFIED_DRIVER_BUILD.md) - Build and test
- [HAIKU_INTEGRATION.md](HAIKU_INTEGRATION.md) - Haiku specifics
- [IMPLEMENTATION_COMPLETE.md](IMPLEMENTATION_COMPLETE.md) - Architecture

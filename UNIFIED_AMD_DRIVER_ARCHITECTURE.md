# Unified AMD GPU Driver Architecture

**Status**: Design Document - Comprehensive Refactor  
**Scope**: Multi-generational, Multi-backend Driver  
**Timeline**: Phase 1 (Reference), Phase 2-4 (Implementation)  
**Platforms**: Linux, FreeBSD (drm-kmod), Haiku  
**GPU Support**: VLIW (2000-6000) → RDNA2 (5700 XT, Lucienne, etc.)

---

## Executive Summary

Instead of separate abstractions for SI/CIK, create a **unified, modular AMD GPU driver** that:

1. **Supports entire AMD GPU lineage** (2005-2024)
2. **Dynamic backend selection** (RADV/Vulkan for modern, Mesa/OpenGL for legacy)
3. **Based on drm-kmod structure** as reference architecture
4. **Single driver entry point** with pluggable components
5. **Tested on real hardware** (Lucienne/RDNA2 locally, Wrestler/VLIW on Haiku)

### Your Hardware

```
Local (Linux - Artix):
  GPU: Lucienne (Renoir)
  Device: [1002:164c]
  Architecture: RDNA 2 (modern)
  Capabilities: RADV (Vulkan) native support
  Role: Testing modern path

Remote (Haiku):
  GPU: Radeon HD 7290 (Wrestler)
  Device: [1002:9806]
  Architecture: VLIW2 (legacy)
  Capabilities: OpenGL only
  Role: Testing legacy path
```

---

## Design Philosophy

```
┌────────────────────────────────────────────────────────────┐
│         UNIFIED AMD GPU DRIVER ARCHITECTURE                │
├────────────────────────────────────────────────────────────┤
│                                                            │
│  Device Probe & Detection                                │
│  ├─ PCI device ID matching                               │
│  ├─ Hardware capabilities assessment                      │
│  └─ Generation classification (VLIW/SI/CIK/GCN/RDNA)     │
│                                                            │
│  GPU Abstraction Layer (HAL)                             │
│  ├─ Abstract IP block management                          │
│  ├─ Unified register access                              │
│  └─ Common memory management                             │
│                                                            │
│  Generation-Specific Handlers                            │
│  ├─ VLIW handler (GFX, DCE, memory)                      │
│  ├─ GCN handler (SI/CIK/Polaris/Vega)                   │
│  └─ RDNA handler (RDNA/RDNA2)                           │
│                                                            │
│  Backend Selection                                        │
│  ├─ RADV (Vulkan) - if GPU supports compute              │
│  ├─ Mesa Gallium (OpenGL) - fallback                     │
│  └─ Software rendering - if hardware disabled            │
│                                                            │
│  Platform Abstraction                                     │
│  ├─ Linux: KMS/DRM interface                             │
│  ├─ FreeBSD: drm-kmod bridge                             │
│  └─ Haiku: Custom accelerant + subsystem                 │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

---

## GPU Generation Map

### Complete AMD GPU Timeline

```
Generation     Timeline   Arch      Codenames                  Driver
───────────────────────────────────────────────────────────────────────
VLIW           2005-2012  VLIW4/2   R700, Northern Islands    radeon
               (legacy)             Wrestler, Zacate

SI (GCN1)      2012-2013  GCN v1    Tahiti, Pitcairn, Verde  radeon/amdgpu
CIK (GCN2)     2013-2014  GCN v2    Bonaire, Hawaii, Kaveri  radeon/amdgpu

GCN3           2015       GCN v3    Fiji, Polaris             amdgpu
GCN4           2016       GCN v4    Vega, Arcturus            amdgpu
GCN5           2019       GCN v5    RDNA                      amdgpu

RDNA           2020       RDNA 1    RX 5700 XT                amdgpu
RDNA2          2020-2021  RDNA 2    RX 6800, Lucienne*        amdgpu ← YOUR HW
RDNA3          2022       RDNA 3    RX 7900 XT                amdgpu

* Lucienne = APU with RDNA 2 integrated GPU
```

### Generation Classification Strategy

```c
enum amd_gpu_generation {
    AMD_VLIW,       // VLIW4/2 (radeon driver only)
    AMD_GCN1,       // Southern Islands
    AMD_GCN2,       // Sea Islands
    AMD_GCN3,       // Fiji, Polaris
    AMD_GCN4,       // Vega
    AMD_GCN5,       // RDNA generation 1
    AMD_RDNA2,      // RDNA generation 2 (your hardware)
    AMD_RDNA3,      // RDNA generation 3
};

enum amd_backend_type {
    AMD_BACKEND_RADV,           // Vulkan (RDNA+ native)
    AMD_BACKEND_MESA_GALLIUM,   // OpenGL (universal fallback)
    AMD_BACKEND_LLVMPIPE,       // Software rendering (last resort)
};
```

---

## Modular Architecture

### Layer 1: Device Detection & Classification

```c
// In: PCI device ID
// Out: GPU info (generation, capabilities, backend)

struct amd_gpu_device {
    uint16_t vendor_id;         // 0x1002 (AMD)
    uint16_t device_id;         // PCI device ID
    
    enum amd_gpu_generation gen;
    const char *codename;       // "Lucienne", "Tahiti", etc.
    const char *marketing_name; // "Radeon RX 6700", etc.
    
    struct {
        bool has_radv;          // Can use RADV (Vulkan)
        bool has_mesa_gallium;  // Can use Mesa (OpenGL)
        bool has_compute;       // Compute shader support
        bool has_raytracing;    // Ray tracing support
    } capabilities;
    
    enum amd_backend_type preferred_backend;
    
    struct amd_gpu_handler *handler;  // Generation-specific functions
};
```

### Layer 2: GPU Handler (Generation-Specific)

```c
struct amd_gpu_handler {
    // Core
    int (*hw_init)(struct amd_device *dev);
    int (*hw_fini)(struct amd_device *dev);
    
    // IP Block Management
    int (*init_ip_blocks)(struct amd_device *dev);
    int (*enable_ip_block)(struct amd_device *dev, int block_type);
    
    // Memory Management
    int (*init_gmc)(struct amd_device *dev);
    int (*allocate_vram)(struct amd_device *dev, size_t size, 
                         uint64_t *gpu_addr);
    
    // Graphics
    int (*init_gfx)(struct amd_device *dev);
    int (*submit_command)(struct amd_device *dev, 
                          struct cmd_buffer *cmd);
    
    // Display
    int (*init_dce)(struct amd_device *dev);
    int (*set_display_mode)(struct amd_device *dev,
                            struct display_mode *mode);
    
    // Cleanup
    void (*cleanup_ip_blocks)(struct amd_device *dev);
};
```

### Layer 3: Backend Abstraction (RADV vs Mesa)

```c
// Backend abstraction for API-specific operations
struct amd_api_backend {
    const char *name;  // "RADV", "Mesa Gallium", "LLVMPipe"
    
    // Shader compilation
    int (*compile_shader)(const char *glsl_source,
                          void **spirv_binary, size_t *size);
    
    // Resource binding
    int (*create_buffer)(struct amd_device *dev, size_t size,
                         struct gpu_buffer **buf);
    int (*bind_texture)(struct amd_device *dev,
                        struct gpu_texture *tex);
    
    // Rendering
    int (*draw_primitives)(struct amd_device *dev,
                           struct draw_call *dc);
    int (*submit_compute_job)(struct amd_device *dev,
                              struct compute_kernel *kernel);
};
```

### Layer 4: Platform Bridge

```c
// Abstraction for Linux KMS, FreeBSD drm-kmod, Haiku subsystem
struct amd_platform_bridge {
    // Device registration
    int (*register_device)(struct amd_device *dev);
    int (*unregister_device)(struct amd_device *dev);
    
    // Memory management
    int (*request_vram)(size_t size, uint64_t *gpu_addr);
    int (*release_vram)(uint64_t gpu_addr);
    
    // Display integration
    int (*set_framebuffer)(uint64_t gpu_addr, uint32_t width,
                           uint32_t height, uint32_t stride);
    
    // Power management
    int (*enter_power_state)(int power_level);
    int (*exit_power_state)(void);
};
```

---

## Device Database Structure

### Database File Format

```
// src/amd/amd_devices.c - Complete GPU device database

struct amd_device_entry {
    uint16_t device_id;
    enum amd_gpu_generation generation;
    const char *codename;
    const char *marketing_name;
    uint32_t max_compute_units;
    uint32_t max_vram;
    struct {
        bool radv_support;
        bool mesa_support;
        bool raytracing;
    } capabilities;
    struct amd_gpu_handler *handler;
};

static const struct amd_device_entry amd_devices[] = {
    // VLIW
    { 0x9806, AMD_VLIW, "Wrestler", "Radeon HD 7290", 
      2, 2048, {false, true, false}, &vliw_handler },
    
    // SI (Southern Islands)
    { 0x6798, AMD_GCN1, "Tahiti", "Radeon HD 7970",
      32, 3072, {false, true, false}, &gcn1_handler },
    
    // RDNA2 (Your hardware)
    { 0x164c, AMD_RDNA2, "Lucienne", "Radeon",
      8, 2048, {true, true, true}, &rdna2_handler },
};
```

---

## Implementation Roadmap

### Phase 1: Architecture & Reference (Current)

**Output**: This document + reference implementation

```
AMDGPU_Abstracted/
├── src/amd/
│   ├── amd_device.h/c         # Core device abstraction
│   ├── amd_handlers.h/c       # Generation-specific handlers
│   ├── amd_devices_db.c       # Complete device database
│   ├── amd_backends.h/c       # RADV/Mesa backend abstraction
│   └── amd_platform.h/c       # Linux/FreeBSD/Haiku bridge
│
├── handlers/
│   ├── vliw_handler.c         # VLIW implementation
│   ├── gcn1_handler.c         # SI (GCN1) implementation
│   ├── rdna2_handler.c        # RDNA2 implementation
│   └── ...
│
└── docs/
    └── UNIFIED_AMD_DRIVER_ARCHITECTURE.md
```

### Phase 2: Linux Integration (1-2 weeks)

- [ ] Integrate with Linux DRM/KMS
- [ ] Test with Lucienne (RDNA2) locally
- [ ] RADV (Vulkan) path
- [ ] Mesa (OpenGL) fallback

### Phase 3: FreeBSD/drm-kmod Port (1 week)

- [ ] Port to drm-kmod bridge
- [ ] Test compilation and basic functionality
- [ ] Reference drm-kmod code as needed

### Phase 4: Haiku & Legacy Support (2-3 weeks)

- [ ] VLIW handler implementation
- [ ] Haiku accelerant integration
- [ ] Test with Wrestler (VLIW)
- [ ] SI/CIK support completion

### Phase 5: Testing & Hardening (1-2 weeks)

- [ ] Full test suite across all generations
- [ ] Dual-system testing (Linux + Haiku)
- [ ] Performance optimization
- [ ] Edge case handling

---

## Driver Entry Points

### Linux Entry Point (amdgpu_pci_probe equivalent)

```c
// src/amd/amd_pci.c

int amd_pci_probe(struct pci_dev *pdev, ...) {
    struct amd_gpu_device gpu_info;
    
    // Step 1: Detect GPU
    if (amd_detect_gpu(pdev->device, &gpu_info) < 0) {
        return -ENODEV;  // Not an AMD GPU we support
    }
    
    // Step 2: Check if we should handle it
    if (amd_should_defer_to_legacy_driver(&gpu_info)) {
        dev_info(&pdev->dev, "%s support provided by %s driver",
                 gpu_info.codename, gpu_info.preferred_driver);
        return -ENODEV;  // Let radeon or amdgpu handle it
    }
    
    // Step 3: Initialize device
    struct amd_device *dev = amd_device_alloc(&gpu_info);
    if (!dev) return -ENOMEM;
    
    if (dev->handler->hw_init(dev) < 0) {
        amd_device_free(dev);
        return -ENODEV;
    }
    
    // Step 4: Select and initialize backend
    if (gpu_info.capabilities.radv_support) {
        dev->backend = amd_backend_radv_init(dev);
    } else {
        dev->backend = amd_backend_mesa_init(dev);
    }
    
    if (!dev->backend) {
        dev->handler->hw_fini(dev);
        amd_device_free(dev);
        return -ENODEV;
    }
    
    // Step 5: Register with platform
    if (amd_platform_register_device(dev) < 0) {
        dev->backend->cleanup(dev);
        dev->handler->hw_fini(dev);
        amd_device_free(dev);
        return -ENODEV;
    }
    
    dev_info(&pdev->dev, "AMD %s (%s) [%04x:%04x] initialized with %s backend",
             gpu_info.marketing_name, gpu_info.codename,
             gpu_info.vendor_id, gpu_info.device_id,
             dev->backend->name);
    
    return 0;
}
```

### FreeBSD/drm-kmod Integration

```c
// Reference drm-kmod structure in:
// /drm-kmod/drivers/gpu/drm/amd/amdgpu/amdgpu_drv.c

// Adapt to use amd_device abstraction instead of
// directly implementing hardware init
```

### Haiku Accelerant Entry Point

```c
// src/os/haiku/accelerant/AmdAccelerant.cpp

status_t get_accelerant_hook(uint32_t feature, void **hook) {
    // Query from amd_device based on detected GPU
    struct amd_device *dev = haiku_get_amd_device();
    
    if (!dev) return B_NO_MEMORY;
    
    switch (feature) {
        case B_ACCELERANT_ENTER:
            *hook = (void *)amd_accelerant_enter;
            return B_OK;
            
        case B_SCREEN_LIST:
            *hook = (void *)amd_screen_list;
            return B_OK;
            
        case B_GET_TIMING_CONSTRAINTS:
            // Delegate to handler
            if (dev->handler->get_timing_constraints) {
                *hook = (void *)dev->handler->get_timing_constraints;
                return B_OK;
            }
            break;
    }
    
    return B_BAD_VALUE;
}
```

---

## Backend Strategy: RADV vs Mesa

### Automatic Backend Selection

```c
enum amd_backend_type amd_select_backend(struct amd_gpu_device *gpu) {
    // Modern GPUs (RDNA+) prefer RADV for Vulkan
    if (gpu->gen >= AMD_RDNA && gpu->capabilities.radv_support) {
        return AMD_BACKEND_RADV;  // Vulkan native
    }
    
    // Legacy/fallback to Mesa for OpenGL
    if (gpu->capabilities.mesa_support) {
        return AMD_BACKEND_MESA_GALLIUM;  // OpenGL via Gallium
    }
    
    // Last resort: Software rendering
    return AMD_BACKEND_LLVMPIPE;  // Software only
}
```

### RADV Implementation (Modern GPUs like Lucienne)

```c
struct amd_api_backend radv_backend = {
    .name = "RADV",
    .compile_shader = radv_compile_spirv,
    .create_buffer = radv_create_buffer,
    .bind_texture = radv_bind_texture,
    .draw_primitives = radv_draw,
    .submit_compute_job = radv_compute_dispatch,
    .cleanup = radv_cleanup,
};
```

### Mesa Gallium Implementation (Legacy GPUs like Warrior)

```c
struct amd_api_backend mesa_backend = {
    .name = "Mesa Gallium",
    .compile_shader = gallium_compile_glsl,
    .create_buffer = gallium_create_buffer,
    .bind_texture = gallium_bind_texture,
    .draw_primitives = gallium_draw,
    .submit_compute_job = NULL,  // Not supported in OpenGL
    .cleanup = gallium_cleanup,
};
```

---

## Module Parameter Strategy

```c
// Global parameters controlling driver behavior

int amd_enable_radv = 1;           // Use RADV for modern GPUs
int amd_enable_mesa = 1;           // Use Mesa for legacy GPUs
int amd_enable_vliw = 1;           // Support VLIW GPUs
int amd_enable_gcn = 1;            // Support GCN GPUs
int amd_enable_rdna = 1;           // Support RDNA GPUs

int amd_software_rendering = 0;    // Force software rendering (testing)
int amd_disable_gpu = 0;           // Disable GPU entirely

// Platform-specific
#ifdef __linux__
int amd_enable_kms = 1;            // Use DRM/KMS on Linux
#endif

#ifdef __FreeBSD__
int amd_drm_kmod_compat = 1;       // Use drm-kmod compatibility layer
#endif

#ifdef __HAIKU__
int amd_enable_accelerant = 1;     // Use custom accelerant
#endif
```

---

## Testing Strategy

### Multi-Platform Testing Matrix

```
┌─────────────────┬────────────────┬─────────────────┬─────────────┐
│ Platform        │ GPU            │ Generation      │ Backend     │
├─────────────────┼────────────────┼─────────────────┼─────────────┤
│ Linux (Local)   │ Lucienne       │ RDNA2 (modern)  │ RADV        │
│ Haiku (Remote)  │ Warrior        │ VLIW (legacy)   │ Mesa OpenGL │
│ FreeBSD (test)  │ Various        │ SI/CIK/RDNA     │ Auto-select │
│ Headless        │ N/A            │ N/A             │ LLVMPipe    │
└─────────────────┴────────────────┴─────────────────┴─────────────┘
```

### Test Suite Structure

```
tests/
├── device_detection/
│   ├── test_lucienne_detection.c
│   ├── test_warrior_detection.c
│   └── test_unknown_device.c
│
├── backend_selection/
│   ├── test_radv_selection.c
│   ├── test_mesa_selection.c
│   └── test_fallback_selection.c
│
├── hardware_init/
│   ├── test_rdna2_init.c
│   ├── test_vliw_init.c
│   └── test_gcn_init.c
│
└── rendering/
    ├── test_simple_triangle.c
    ├── test_vulkan_compute.c
    └── test_opengl_basic.c
```

---

## Key Benefits of This Approach

1. **Single Driver** - All AMD GPUs in one modular driver
2. **Automatic Backend** - RADV for modern, Mesa for legacy, software fallback
3. **Testable** - Can test modern (Lucienne) and legacy (Warrior) simultaneously
4. **Extensible** - Easy to add new GPU generations
5. **Cross-Platform** - Linux, FreeBSD, Haiku with same codebase
6. **Reference** - drm-kmod as architectural reference
7. **Production-Ready** - Based on proven kernel driver patterns

---

## Reference: drm-kmod Structure

The FreeBSD drm-kmod project provides excellent reference for:

- **Device detection** pattern
- **Handler abstraction** for different GPU families
- **IP block management** system
- **Memory management** layer
- **Integration patterns** with platform subsystems

Key files in drm-kmod to reference:
```
drivers/gpu/drm/amd/amdgpu/amdgpu_drv.c     # Main driver entry
drivers/gpu/drm/amd/amdgpu/amdgpu_device.c  # Device management
drivers/gpu/drm/amd/amdgpu/amdgpu.h         # Core structures
drivers/gpu/drm/amd/amdgpu/amdgpu_gfx.c     # Graphics handling
drivers/gpu/drm/amd/amdgpu/amdgpu_display.c # Display handling
```

---

## Next Steps

1. **Create comprehensive device database** (all AMD GPUs)
2. **Design handler abstraction** for each generation
3. **Implement backend selection logic**
4. **Port RADV integration** for Lucienne
5. **Port Mesa integration** for VLIW
6. **Test on both systems** simultaneously
7. **Document integration patterns**
8. **Prepare for upstream**

---

**Status**: Design phase complete  
**Ready for**: Phase 2 implementation  
**Estimated Timeline**: 4-6 weeks for full implementation  
**Current Hardware**: Lucienne (RDNA2) + Warrior (VLIW)

# AMD Unified GPU Driver - Architecture Redesign Proposal

**Date**: January 17, 2026  
**Status**: Design Consultation Document  
**Audience**: Development Team & Implementation Agent  
**Priority**: Critical - Consolidation & Hardware Integration

---

## Executive Summary

The current unified driver framework is **architecturally sound but incomplete**. It provides excellent abstraction for device detection, backend selection, and multi-GPU management, but **fails to initialize real hardware** and **conflicts with existing codebase**.

This document proposes a **unified, consolidated architecture** that:
1. **Merges** the new unified driver with existing HAL/IP blocks
2. **Eliminates** redundant abstractions (3 parallel systems)
3. **Connects** handlers to real hardware registers
4. **Provides** a clear path from abstraction → IP blocks → register access
5. **Maintains** the strength of device detection and backend selection

---

## Problem Analysis

### Current Architecture (Broken)

```
Application Layer
    ↓
THREE COMPETING ABSTRACTIONS:
    ├─ Unified Driver (New)
    │   └─ amd_device.h → handlers → backends
    │       STATUS: Complete abstraction, ZERO hardware access
    │
    ├─ HAL Layer (Existing)
    │   └─ hal.h → amd_ip_funcs → ?
    │       STATUS: Incomplete, expects IP blocks
    │
    └─ Legacy Objects (Existing)
        └─ objgpu.c → OBJGPU → rmapi
            STATUS: Compiled but not used

HARDWARE REGISTERS
    └─ asic_reg/ (~500 header files)
        STATUS: Defined but disconnected
```

### Key Problems

| Problem | Impact | Severity |
|---------|--------|----------|
| No hardware initialization | GPU never boots | CRITICAL |
| 3 competing architectures | Code conflicts, confusion | CRITICAL |
| IP blocks not registered | No capability management | CRITICAL |
| Mock memory management | IOMMU never set up | CRITICAL |
| Display never configured | No output possible | HIGH |
| Duplicate abstractions | Code duplication, maintenance burden | HIGH |
| Register files unused | 500+ files doing nothing | MEDIUM |

---

## Proposed Solution: Unified Architecture

### Overview

```
Consolidated Single Path:

Application/OS Layer
    ↓
AMD Unified Driver (Enhanced)
├─ Device Detection & Classification
├─ Backend Selection (RADV/Mesa/Zink)
├─ Resource Manager (RsResource)
└─ Abstract Handler Interface
    ↓
Generation-Specific Handlers (VLIW/GCN/RDNA)
├─ Call IP Block Initializers
├─ Manage register access patterns
└─ Coordinate hardware initialization
    ↓
IP Block Managers (Real Hardware)
├─ gfx_v10.c / gfx_v9.c → GPU engine
├─ gmc_v10.c / gmc_v8.c → Memory controller
├─ clock_v10.c → Clock control
├─ display_v10.c → Display engine (NEW)
└─ sdma_v4.c → DMA engines
    ↓
Register Access Layer
├─ asic_reg/dce/ → Display registers
├─ asic_reg/gmc/ → Memory registers
├─ asic_reg/gc/ → Graphics registers
└─ asic_reg/sdma/ → DMA registers
    ↓
MMIO Interface
    ↓
REAL HARDWARE
```

### Design Principles

1. **Single Authority**: One path from driver to hardware
2. **Layered**: Each layer has clear responsibility
3. **Pluggable**: Handlers route to appropriate IP blocks
4. **Register-Driven**: All hardware access via asic_reg definitions
5. **Multi-Platform**: Works on Linux, Haiku, FreeBSD
6. **Testable**: Each layer independently testable

---

## Detailed Architecture

### Layer 1: Device Detection (KEEP AS-IS)

**File**: `src/amd/amd_device.h / amd_devices.c / amd_device_core.c`

**Responsibility**: Identify GPU and select handler

```c
// Probe flow - THIS PART WORKS WELL
amd_device_probe(device_id)
    ↓ lookup in amd_device_database
    ↓ classify generation (VLIW/GCN/RDNA)
    ↓ select appropriate handler
    ↓ select preferred backend
    ↓ return amd_device struct

// What changes: After probe, handler != printf
```

**No changes needed** - this layer is correct.

---

### Layer 2: Handler Interface (REDESIGN)

**File**: `src/amd/amd_handler.h` (rename from amd_device.h's embedded struct)

**Current Problem**: Handlers are just printf stubs

**Redesigned Interface**:

```c
typedef struct amd_gpu_handler {
    const char *name;
    amd_gpu_generation_t generation;
    
    // Core initialization - CALLS IP BLOCKS
    int (*probe)(amd_device_t *dev);              // New: gather hw info
    int (*init_pre_hardware)(amd_device_t *dev);  // Setup before HW
    int (*init_hardware)(amd_device_t *dev);      // ACTUAL hardware init
    int (*init_post_hardware)(amd_device_t *dev); // Verification
    
    // IP Block Management - ROUTE TO REAL IP BLOCKS
    struct {
        struct amd_ip_block *gmc;   // Graphics Memory Controller
        struct amd_ip_block *gfx;   // Graphics Engine
        struct amd_ip_block *display; // Display Engine
        struct amd_ip_block *sdma;  // DMA Engines
        struct amd_ip_block *clock; // Clock/Power
        // ... more as needed
    } ip_blocks;
    
    // Lifecycle
    int (*suspend)(amd_device_t *dev);
    int (*resume)(amd_device_t *dev);
    int (*fini)(amd_device_t *dev);
    
    // Hardware operations - DELEGATES TO IP BLOCKS
    int (*allocate_vram)(amd_device_t *dev, size_t size, uint64_t *gpu_addr);
    int (*set_display_mode)(amd_device_t *dev, const display_mode *mode);
    int (*submit_command)(amd_device_t *dev, void *cmd);
    
} amd_gpu_handler_t;
```

**Implementation Strategy**:

Each handler points to IP blocks rather than implementing directly:

```c
// vliw_handler.c
static int vliw_init_hardware(amd_device_t *dev) {
    amd_ip_block_gfx_vliw_init(dev);     // Call REAL IP block
    amd_ip_block_gmc_vliw_init(dev);     // Call REAL IP block
    amd_ip_block_display_dce_init(dev);  // Call REAL IP block
    return 0;
}

// rdna_handler.c
static int rdna_init_hardware(amd_device_t *dev) {
    amd_ip_block_gfx_v10_init(dev);      // Call REAL IP block
    amd_ip_block_gmc_v10_init(dev);      // Call REAL IP block
    amd_ip_block_display_dcn_init(dev);  // Call REAL IP block
    return 0;
}
```

---

### Layer 3: IP Block Interface (STANDARDIZE)

**File**: `src/amd/ip_blocks/ip_block.h` (new)

**Current Problem**: IP blocks exist (gfx_v10.c, gmc_v10.c) but no unified interface

**Standardized Interface**:

```c
typedef struct amd_ip_block_ops {
    const char *name;
    uint32_t version;
    
    // Lifecycle
    int (*early_init)(amd_device_t *dev);  // Detect capability
    int (*init)(amd_device_t *dev);        // Hardware init
    int (*fini)(amd_device_t *dev);        // Shutdown
    
    // Optional
    int (*suspend)(amd_device_t *dev);
    int (*resume)(amd_device_t *dev);
    int (*reset)(amd_device_t *dev);
    
    // Status
    int (*is_idle)(amd_device_t *dev);
    int (*wait_idle)(amd_device_t *dev);
    
} amd_ip_block_ops_t;

typedef struct amd_ip_block {
    amd_ip_block_ops_t *ops;
    void *data;  // IP-block-specific state
    int enabled;
} amd_ip_block_t;
```

**Registry System**:

```c
// Register IP blocks with device
int amd_device_register_ip_block(
    amd_device_t *dev,
    amd_ip_block_t *block
);

// Query registered blocks
amd_ip_block_t* amd_device_get_ip_block(
    amd_device_t *dev,
    const char *name
);
```

**Existing IP Blocks - UPDATE TO USE INTERFACE**:

```c
// src/amd/ip_blocks/gfx_v10.c (simplified)

static int gfx_v10_init(amd_device_t *dev) {
    // REAL hardware initialization
    // Access asic_reg/gc/gc_10_*.h definitions
    amd_mmio_write(dev, GC_10_CNTL, 0x00000001);
    // ... actual register writes
    return 0;
}

static amd_ip_block_ops_t gfx_v10_ops = {
    .name = "GFX v10",
    .version = 0x0a0001,
    .init = gfx_v10_init,
    // ...
};
```

---

### Layer 4: Register Access (USE EXISTING)

**Files**: `src/amd/include/asic_reg/` (500+ header files)

**Current**: Defined but not used

**Solution**: IP blocks include and use these

```c
// In gfx_v10.c
#include "../../include/asic_reg/gc/gc_10_0_offset.h"
#include "../../include/asic_reg/gc/gc_10_0_sh_mask.h"

// Use register definitions
#define GC_10_REG_MMIO_CNTL      (mmGC_MMIO_CNTL)
#define GC_10_REG_COMPUTE_START  (mmGC_USER_COMPUTE_START)

// Access via abstracted write function
static inline void write_gc_reg(amd_device_t *dev, uint32_t offset, uint32_t val) {
    // Bounds check, logging, etc.
    *(uint32_t*)(dev->mmio_base + offset) = val;
}

// Use in init
write_gc_reg(dev, GC_10_CNTL_OFFSET, GC_10_CNTL__SPI_COMPUTE_ENABLE_MASK);
```

---

### Layer 5: MMIO Interface (ABSTRACT)

**File**: `src/amd/hal/mmio.h` (new)

**Provides**: Safe hardware register access

```c
typedef struct amd_mmio_ops {
    uint32_t (*read)(amd_device_t *dev, uint32_t offset);
    void (*write)(amd_device_t *dev, uint32_t offset, uint32_t val);
    void (*rmw)(amd_device_t *dev, uint32_t offset, uint32_t mask, uint32_t val);
} amd_mmio_ops_t;

// Default implementation
static uint32_t amd_mmio_read(amd_device_t *dev, uint32_t offset) {
    if (offset >= dev->mmio_size) return 0xDEADBEEF;  // Error marker
    return *(uint32_t*)(dev->mmio_base + offset);
}

static void amd_mmio_write(amd_device_t *dev, uint32_t offset, uint32_t val) {
    if (offset >= dev->mmio_size) return;
    *(uint32_t*)(dev->mmio_base + offset) = val;
}

// Can be overridden for:
// - Simulation/testing
// - Logging/tracing
// - Platform-specific access (Haiku vs Linux)
```

---

## Integration Points

### Backend Detection (KEEP, IMPROVE)

**File**: `src/amd/amd_backend_detection.h/c`

**Current**: Works well

**Improvement**: Integrate with handler

```c
// In handler->init_hardware()
amd_backend_support_t support;
amd_detect_backend_support(&support);
amd_setup_backend_env(&support);

// Now hardware is ready and backend is configured
```

### Resource Manager (NEW/INTEGRATE)

**File**: `src/amd/resource/resource_manager.h` (new)

**Purpose**: Track allocations (VRAM, buffers, etc.)

```c
typedef struct amd_resource_manager {
    struct RsResource *root;
    size_t total_vram;
    size_t vram_used;
    // ... lock for sync
} amd_resource_manager_t;

int amd_resource_alloc(
    amd_device_t *dev,
    size_t size,
    amd_resource_handle_t *handle
);

int amd_resource_free(
    amd_device_t *dev,
    amd_resource_handle_t handle
);
```

**Used by**: Handlers when allocating VRAM

---

### Display Integration (NEW)

**File**: `src/amd/ip_blocks/display.h/c` (new)

**Purpose**: Unified display initialization

```c
// Choose between DCE (legacy) or DCN (modern) automatically
int amd_ip_block_display_init(amd_device_t *dev) {
    if (dev->gpu_info.generation <= AMD_GCN4) {
        return dce_init(dev);  // DCE for older GPUs
    } else {
        return dcn_init(dev);  // DCN for RDNA
    }
}

static int dce_init(amd_device_t *dev) {
    // Use asic_reg/dce/ definitions
    // Actually write DCE registers
}

static int dcn_init(amd_device_t *dev) {
    // Use asic_reg/dcn/ definitions
    // Actually write DCN registers
}
```

---

## Data Flow Example: Device Initialization

```
amd_device_init(dev)
    ↓
detect_backend_support(dev)
    ↓ [detects RADV/Mesa/Zink available]
setup_backend_env(dev)
    ↓ [sets environment variables]
dev->handler->probe(dev)
    ↓ [gathers HW info without touching hardware]
dev->handler->init_pre_hardware(dev)
    ↓ [setup, enable clocks, etc]
dev->handler->init_hardware(dev)
    ├─ ip_blocks[GMC]->init(dev)
    │   ↓
    │   write_gmc_reg(dev, GMC_VMID0_ADDR, ...)
    │   [actual register writes]
    │
    ├─ ip_blocks[GFX]->init(dev)
    │   ↓
    │   write_gc_reg(dev, GC_CNTL, ...)
    │   [actual register writes]
    │
    └─ ip_blocks[DISPLAY]->init(dev)
        ↓
        if (gen <= GCN4) dce_init()
        else dcn_init()
        [actual DCE/DCN register writes]
        ↓
dev->handler->init_post_hardware(dev)
    ↓ [verify HW state]
return 0
```

---

## File Organization (Proposed)

```
src/amd/
├── amd_device.h              (KEEP - core API)
├── amd_devices.c             (KEEP - device database)
├── amd_device_core.c         (UPDATE - use new handler interface)
├── amd_handler.h             (NEW - standardized handler interface)
├── amd_backend_detection.h   (KEEP - works well)
├── amd_backend_detection.c   (KEEP)
│
├── handlers/
│   ├── vliw_handler.c        (UPDATE - call IP blocks)
│   ├── gcn_handler.c         (UPDATE - call IP blocks)
│   └── rdna_handler.c        (UPDATE - call IP blocks)
│
├── ip_blocks/
│   ├── ip_block.h            (NEW - standardized interface)
│   ├── gfx_v10.c             (UPDATE - implement init())
│   ├── gmc_v10.c             (UPDATE - implement init())
│   ├── clock_v10.c           (UPDATE - implement init())
│   ├── display.c             (NEW - unified display)
│   ├── dce_v10.c             (NEW - DCE support)
│   ├── dcn_v1.c              (NEW - DCN support)
│   └── sdma_v4.c             (UPDATE - implement init())
│
├── resource/
│   └── resource_manager.h    (NEW - track allocations)
│
├── hal/
│   ├── mmio.h                (NEW - MMIO abstraction)
│   ├── mmio.c                (NEW - implementations)
│   └── hal.h                 (KEEP for compatibility, wrap with amd_device)
│
└── include/
    └── asic_reg/             (KEEP - register definitions)
        ├── dce/
        ├── dcn/
        ├── gmc/
        ├── gc/
        └── ... (all 500+ files)

❌ DELETE:
├── radeon_legacy_support.c   (REPLACED BY HANDLERS)
├── radv_backend_v2.c         (REPLACE WITH DETECTION + HANDLER)
├── shader_compiler_v2.c      (NOT NEEDED AT DRIVER LEVEL)
├── rmapi/                    (REDUNDANT ARCHITECTURE)
├── zink_layer/               (HANDLED BY BACKEND DETECTION)
└── vulkan/                   (NOT DRIVER RESPONSIBILITY)
```

---

## Initialization Sequence (Detailed)

### Phase 1: Probe (Lightweight)

```c
amd_device_probe(0x164c)
    ↓
lookup_device_database("0x164c")
    → returns { gen: RDNA2, handler: rdna_handler, ... }
    ↓
rdna_handler->probe(dev)  // NEW: gather capabilities
    → read asic_reg info (non-destructive)
    → detect GPU clock speed
    → detect memory size
    → return capabilities
    ↓
amd_select_backend(dev)
    ↓
amd_device_t returned with handler + backend selected
```

### Phase 2: Hardware Init (Destructive)

```c
amd_device_init(dev)
    ↓
amd_detect_backend_support()
    → check RADV/Mesa/Zink available
    → setup environment variables (Linux: suggest, Haiku: set)
    ↓
dev->handler->init_pre_hardware(dev)
    → disable interrupts
    → reset GPU
    → enable clocks
    ↓
dev->handler->init_hardware(dev)
    ├─ gmc->init(dev)    // Initialize memory controller
    │   → setup IOMMU
    │   → configure address translation
    │   → validate VRAM
    │
    ├─ gfx->init(dev)    // Initialize graphics engine
    │   → setup command processor
    │   → initialize compute units (if GCN+)
    │   → load microcode
    │
    ├─ display->init(dev)  // Initialize display
    │   ├─ if (gen <= GCN4) dce_init()  // Old display
    │   └─ else dcn_init()              // New display
    │
    ├─ clock->init(dev)  // Clock management
    │   → set power policy
    │   → configure frequency scaling
    │
    └─ sdma->init(dev)   // DMA engines
        → setup transfer rings
        → configure DMA engines
        ↓
dev->handler->init_post_hardware(dev)
    → verify hardware state
    → sanity checks
    ↓
DEVICE READY
```

---

## Backend Detection Integration

### Linux Behavior

```c
amd_detect_backend_support(dev)
    → check for RADV: dlopen("libvulkan.so")
    → check for Mesa: dlopen("libGL.so")
    → check for Zink: dlopen("libzink.so")
    ↓
amd_setup_backend_env(dev)
    [On Linux - only suggest]
    printf("export AMD_GPU_BACKEND=radv\n");
    printf("export VK_DRIVER_FILES=...\n");
    // Do NOT call setenv()
    ↓
User can copy-paste or ignore recommendations
```

### Haiku Behavior

```c
amd_detect_backend_support(dev)
    → check for RADV: dlopen("libvulkan.so")
    → check for Mesa: dlopen("libGL.so") [likely Mesa port]
    → check for Zink: dlopen("libzink.so") [unlikely]
    ↓
amd_setup_backend_env(dev)
    [On Haiku - automatically configure]
    setenv("AMD_GPU_BACKEND", "gallium", 1);
    setenv("LIBGL_ALWAYS_INDIRECT", "1", 1);
    // System is ready immediately
    ↓
Accelerant can use configured backend
```

---

## Multi-GPU Support

```c
Device 1 (Lucienne RDNA2)     Device 2 (Warrior VLIW)
    ↓                              ↓
amd_device_probe(0x164c)      amd_device_probe(0x9806)
    ↓                              ↓
rdna_handler assigned         vliw_handler assigned
    ↓                              ↓
amd_device_init(dev1)         amd_device_init(dev2)
    ├─ rdna hw init                ├─ vliw hw init
    ├─ ip_blocks[gfx_v10]          ├─ ip_blocks[gfx_vliw]
    ├─ ip_blocks[gmc_v10]          ├─ ip_blocks[gmc_vliw]
    ├─ ip_blocks[dcn_init]         ├─ ip_blocks[dce_init]
    └─ RADV backend setup          └─ Mesa backend setup
        ↓                              ↓
    Ready for Vulkan            Ready for OpenGL
```

---

## Testing Strategy

### Unit Tests (Per-Component)

```c
// test_vliw_handler.c
test_vliw_handler_probe()
    → Mock mmio
    → Call handler->probe()
    → Verify capability detection

test_vliw_handler_init_hardware()
    → Mock mmio with register check
    → Call handler->init_hardware()
    → Verify register writes in correct order

// test_gmc_v10.c
test_gmc_v10_init()
    → Mock MMIO
    → Call gmc_v10->init()
    → Verify memory controller registers

// test_display_dce.c
test_display_dce_init()
    → Mock MMIO
    → Call display_dce->init()
    → Verify DCE register writes
```

### Integration Tests

```c
// test_lucienne_complete.c
test_lucienne_initialization()
    → Probe device
    → Init device
    → Verify all IP blocks initialized
    → Verify backend selected
    → Verify display ready

// test_multi_gpu.c
test_dual_gpu_independent()
    → Init two different GPUs
    → Verify independent state
    → Verify no cross-contamination
```

### Platform Tests

```c
// test_linux_backend_suggestion.c
test_linux_suggests_backend()
    → Mock dlopen for RADV available
    → Call backend detection
    → Verify stdout suggests export
    → Verify setenv NOT called

// test_haiku_backend_auto.c
test_haiku_auto_backend()
    → Mock dlopen for Mesa available
    → Call backend detection
    → Verify setenv called
    → Verify env vars set in process
```

---

## Migration Path

### Phase 1: Consolidation (Week 1-2)

1. Create `amd_handler.h` with standardized interface
2. Create `ip_blocks/ip_block.h` with standardized IP block interface
3. Update existing IP blocks (gfx_v10, gmc_v10) to implement interface
4. Update handlers (vliw, gcn, rdna) to call IP blocks
5. Create registry system for IP block discovery

### Phase 2: Hardware Integration (Week 2-3)

1. Create display.c with unified display init
2. Create dce_v10.c and dcn_v1.c implementations
3. Add register access via asic_reg headers
4. Test each IP block independently
5. Test full initialization sequence

### Phase 3: Cleanup (Week 3)

1. Remove legacy code (radeon_legacy_support, radv_backend_v2)
2. Remove redundant abstractions (rmapi, zink_layer if not needed)
3. Consolidate device database (single source of truth)
4. Add comprehensive testing

### Phase 4: Deployment (Week 4)

1. Test on Linux with Lucienne
2. Cross-compile and test on Haiku with Warrior
3. Integration with Haiku accelerant
4. Documentation updates

---

## Key Improvements

| Aspect | Current | Proposed | Benefit |
|--------|---------|----------|---------|
| **Architecture** | 3 competing systems | 1 unified path | No conflicts |
| **Hardware Init** | Mock (printf) | Real register writes | Actual HW works |
| **IP Blocks** | Unused headers | Active initialization | Capabilities used |
| **Code Duplication** | 4 backend abstractions | 1 detection + selection | Maintainability |
| **Device Database** | 2 sources | 1 amd_devices.c | Single source of truth |
| **Display** | 3 locations trying | 1 unified system | Works correctly |
| **Register Access** | 500 files unused | Used by IP blocks | Organized access |
| **Testing** | Unit tests only | Unit + integration | Verified integration |

---

## Open Questions Resolved

### Q: What happens to radeon_legacy_support.c?
**A**: Removed. VLIW handler + dce_v10.c handles all VLIW initialization.

### Q: What about shader compilation?
**A**: Not driver responsibility. Handled by RADV/Mesa libraries after device init.

### Q: How do we handle different DCE versions?
**A**: Handler selects at init time:
```c
if (dev->gpu_info.generation <= GCN4) {
    ip_blocks[DISPLAY] = &dce_v10_ops;  // DCE
} else {
    ip_blocks[DISPLAY] = &dcn_v1_ops;   // DCN
}
```

### Q: What if display_mode struct doesn't match?
**A**: Use Haiku's real definition. Fallback only if not available.

### Q: How do we test without real hardware?
**A**: Mock MMIO interface. Can verify register writes without actual GPU.

---

## Conclusion

This redesigned architecture provides:

1. **Single authority** - One path, clear responsibility hierarchy
2. **Hardware integration** - Real register access via IP blocks
3. **Elimination of conflicts** - No more 3 competing systems
4. **Foundation for Haiku** - Ready for accelerant integration
5. **Extensibility** - Easy to add new GPU generations or IP blocks
6. **Testability** - Each layer independently testable

The unified driver framework's core strengths (device detection, backend selection, multi-GPU support) are preserved and enhanced with real hardware initialization capability.

---

## Implementation Checklist

- [ ] Create amd_handler.h interface
- [ ] Create ip_block.h interface
- [ ] Update gfx_v10.c to implement interface
- [ ] Update gmc_v10.c to implement interface
- [ ] Create display.c with dce/dcn selection
- [ ] Create dce_v10.c implementation
- [ ] Create dcn_v1.c implementation
- [ ] Update handlers to call IP blocks
- [ ] Create IP block registry
- [ ] Add MMIO abstraction layer
- [ ] Update backend detection integration
- [ ] Create comprehensive tests
- [ ] Test on Linux
- [ ] Test on Haiku
- [ ] Document integration patterns
- [ ] Remove legacy/duplicate code

---

**Document Status**: Complete Design Proposal Ready for Implementation  
**Next Action**: Hand to implementation agent with this document as reference

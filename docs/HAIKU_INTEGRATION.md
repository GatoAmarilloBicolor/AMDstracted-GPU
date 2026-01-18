# Unified AMD Driver - Haiku Integration Guide

## Overview
This guide explains how to integrate the unified AMD driver with Haiku's accelerant system for the Warrior GPU.

## Hardware Target
- **GPU**: Radeon HD 7290 (Warrior)
- **Device ID**: 0x9806
- **Architecture**: VLIW (legacy)
- **Backend**: Mesa OpenGL
- **Display**: DCE (Display Controller Engine)

## Haiku Accelerant Integration

### Architecture
```
Haiku Application
       ↓
OpenGL Driver (libGL.so)
       ↓
Mesa Gallium
       ↓
AMD Driver (libamd_unified_haiku.so)
       ↓
Accelerant (get_accelerant_hook)
       ↓
GPU Hardware (Warrior)
```

### Entry Point Structure

The unified driver provides a clean interface for Haiku accelerants:

```c
// In accelerant/AmdAccelerant.cpp

#include "../../src/amd/amd_device.h"

// Global device state
static amd_device_t *g_amd_device = NULL;

status_t InitAccelerant(int fd) {
    // Open and probe device
    amd_device_t *dev = NULL;
    
    // Probe Warrior (0x9806)
    if (amd_device_probe(0x9806, &dev) != 0) {
        return B_NO_DEVICE;
    }
    
    // Initialize hardware
    if (amd_device_init(dev) != 0) {
        amd_device_free(dev);
        return B_ERROR;
    }
    
    g_amd_device = dev;
    return B_OK;
}

status_t UninitAccelerant(void) {
    if (g_amd_device) {
        amd_device_fini(g_amd_device);
        amd_device_free(g_amd_device);
        g_amd_device = NULL;
    }
    return B_OK;
}

status_t GetAccelerantHook(uint32_t feature, void **hook) {
    if (!g_amd_device) {
        return B_ERROR;
    }
    
    switch (feature) {
        // Display features
        case B_SCREEN_LIST:
            *hook = (void *)AmdScreenList;
            return B_OK;
            
        case B_GET_TIMING_CONSTRAINTS:
            *hook = (void *)AmdGetTimingConstraints;
            return B_OK;
            
        case B_GET_DISPLAY_TIMING:
            *hook = (void *)AmdGetDisplayTiming;
            return B_OK;
            
        // Memory features
        case B_ALLOCATE_GRAPHICS_MEMORY:
            *hook = (void *)AmdAllocateGraphicsMemory;
            return B_OK;
            
        case B_FREE_GRAPHICS_MEMORY:
            *hook = (void *)AmdFreeGraphicsMemory;
            return B_OK;
            
        // Power management
        case B_SET_POWER_STATE:
            *hook = (void *)AmdSetPowerState;
            return B_OK;
    }
    
    return B_BAD_VALUE;
}
```

## Build Instructions for Haiku

### Step 1: Build on Linux
```bash
# In Linux environment (your Artix system)
cd AMDGPU_Abstracted

# Build library for Linux (test)
make -f Makefile.unified clean all run-tests
```

### Step 2: Prepare Haiku Build
```bash
# On Haiku system or with Haiku cross-compiler
scripts/build_for_haiku.sh
```

The script will:
1. Check for Haiku GCC toolchain
2. Compile all sources with Haiku flags
3. Link `libamd_unified_haiku.so`
4. Build test executable
5. Output in `build_haiku/`

### Step 3: Test on Haiku
```bash
# Transfer to Haiku
scp build_haiku/test_unified_driver user@haiku:/tmp/
scp build_haiku/libamd_unified_haiku.so user@haiku:/tmp/

# SSH to Haiku
ssh user@haiku

# Run tests
/tmp/test_unified_driver
```

## Integration Checklist

- [ ] Device probes correctly (0x9806)
- [ ] VLIW handler assigned
- [ ] Mesa backend selected
- [ ] Hardware initializes
- [ ] VRAM allocated successfully
- [ ] Display engine initialized
- [ ] Finalization cleans up properly

## Accelerant Implementation

### Display Mode Setting
```c
status_t AmdSetDisplayMode(
    uint16_t width, 
    uint16_t height, 
    uint16_t depth,
    uint32_t refresh) {
    
    if (!g_amd_device) return B_ERROR;
    
    // Call driver
    int ret = g_amd_device->handler->set_display_mode(
        g_amd_device, width, height);
    
    return (ret == 0) ? B_OK : B_ERROR;
}
```

### Memory Allocation
```c
status_t AmdAllocateGraphicsMemory(
    uint32_t size,
    void **address) {
    
    if (!g_amd_device) return B_ERROR;
    
    uint64_t gpu_addr = 0;
    int ret = g_amd_device->handler->allocate_vram(
        g_amd_device, size, &gpu_addr);
    
    if (ret != 0) return B_ERROR;
    
    *address = (void *)gpu_addr;
    return B_OK;
}
```

### Interrupt Handling
```c
status_t AmdWaitForVBlank(void) {
    // Wait for vertical blank interrupt
    // Implementation depends on DCE registers
    return B_OK;
}
```

## Testing Strategy

### Phase 1: Unit Tests (Local Linux)
✓ Device detection
✓ Backend selection
✓ Handler assignment
✓ Multi-GPU support
✓ Lifecycle management

### Phase 2: Integration Tests (Haiku)
- Accelerant hooks
- Display mode setting
- Memory allocation
- Interrupt handling
- OpenGL rendering

### Phase 3: Functional Tests (Haiku)
- Desktop display
- 2D graphics
- 3D (if available)
- VBlank synchronization

## Known Issues

1. **VLIW Display**: DCE registers differ from modern GPUs
   - Solution: VLIW-specific display initialization in handler

2. **Memory Mapping**: Haiku expects different memory layout
   - Solution: Add Haiku-specific memory manager bridge

3. **Interrupts**: VLIW doesn't support modern interrupt model
   - Solution: Polling-based alternative for legacy GPUs

## Debugging

### Enable Debug Output
```c
// In amd_device.h
#define DEBUG_VLIW 1
#define DEBUG_DISPLAY 1
#define DEBUG_MEMORY 1
```

### Check Hardware Status
```c
// In Haiku
void DebugAmdDevice(void) {
    if (g_amd_device) {
        amd_device_print_info(&g_amd_device->gpu_info);
    }
}
```

### Read DCE Registers
```c
// Warrior DCE register offsets
#define DCE_CRTC_H_TOTAL 0x9000
#define DCE_CRTC_V_TOTAL 0x9020

// Add to vliw_handler.c for real hardware
uint32_t h_total = readl(mmio_base + DCE_CRTC_H_TOTAL);
```

## Future Enhancements

1. **Power Management**
   - CPU to APU power gating
   - Clock scaling

2. **Compute Support**
   - VLIW doesn't support, but framework ready for future

3. **Display Modes**
   - HDMI resolution detection
   - Multi-display support

4. **Performance**
   - Async compute (not on VLIW)
   - DMA optimization

## File Structure

```
AMDGPU_Abstracted/
├── src/amd/amd_device.h           # Core API
├── src/amd/handlers/vliw_handler.c # Warrior implementation
├── HAIKU_INTEGRATION.md            # This file
└── scripts/build_for_haiku.sh      # Build script

Haiku Accelerant (to be created):
├── AmdAccelerant.cpp               # Entry points
├── DisplayController.cpp           # Display modes
├── MemoryManager.cpp               # VRAM allocation
└── RegisterAccess.h                # Hardware register defs
```

## Git Workflow

```bash
# Local: test and verify
make -f Makefile.unified run-tests

# Push to repo
git push origin main

# Haiku: pull and build
scripts/build_for_haiku.sh

# Test on hardware
build_haiku/test_unified_driver
```

## Support Matrix

| GPU | Handler | Backend | Status |
|-----|---------|---------|--------|
| Warrior (7290) | VLIW | Mesa | ✓ Framework ready |
| Lucienne (6700) | RDNA | RADV | ✓ Framework ready |
| RX 480 | GCN3 | Mesa | ✓ Database entry |
| RX Vega | GCN4 | RADV | ✓ Database entry |
| RX 5700 XT | GCN5 | RADV | ✓ Database entry |
| RX 6800 XT | RDNA2 | RADV | ✓ Database entry |

## Documentation Links

- [Unified Driver Build Guide](UNIFIED_DRIVER_BUILD.md)
- [AMD GPU Driver Architecture](ARCHITECTURE_COMPARISON.md)
- [VLIW Legacy Support](RADEON_LEGACY_IMPLEMENTATION_SUMMARY.md)
- [Haiku Accelerant API](https://www.haiku-os.org/legacy-docs/DevelopersGuide/device_drivers/accelerants.html)

## Contact & Status

Status: **Framework Complete, Ready for Haiku Integration**

Next milestone: Run tests on Haiku system with Warrior GPU

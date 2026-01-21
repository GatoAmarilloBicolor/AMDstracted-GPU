# VLIW Legacy GPU Support - Reference Documentation

**Status**: Reference document for future implementation  
**Hardware Target**: Radeon HD 2000-6000 series (pre-GCN)  
**Current Project**: AMDGPU_Abstracted (Phase 1 - SI/CIK only)

---

## Executive Summary

This document provides a reference for supporting VLIW-based AMD GPUs (pre-2012) in the AMDGPU abstraction layer. Currently out of scope for Phase 1 but documented for future extension.

**Key Facts**:
- VLIW = Vector Instructions Long Word (pre-GCN architecture)
- Timeline: ~2005-2012 (Radeon HD 2000-6000 series)
- Free driver support: **Radeon driver** (Linux/FreeBSD only)
- Haiku support: **None (software rendering only)**

---

## Architecture Overview

### VLIW Generation Stack

```
Timeline        Hardware                  Codename          Architecture
──────────────────────────────────────────────────────────────────────────
2005-2009       Radeon HD 2000-4000       RV770, RV730       VLIW (R700)
2010-2011       Radeon HD 5000-6000       Cayman, Turks      VLIW (Northern Islands)
2012            Radeon A-Series (Trinity)  Wrestler/Zacate    VLIW + CPU
2013+           ---                        ---                GCN Era begins
```

### Your Hardware: Radeon HD 7290 (Wrestler)

```
Device ID:     0x9806
Hardware Type: APU (integrated CPU+GPU)
GPU Codename:  Wrestler
VLIW Version:  VLIW4 (4-wide vector)
Memory Type:   DDR3 (shared with CPU)
Display:       HDMI audio + VGA
Release:       2012
Legacy Status: DEPRECATED but still functional
```

---

## Free Driver Landscape

### Linux Kernel Radeon Driver

**Status**: Actively maintained for bug fixes  
**Location**: `drivers/gpu/drm/amd/amdgpu/radeon/`  
**Architecture**: DRM/KMS based  
**Features**:
- 3D graphics (OpenGL 1.3)
- Video decode (UVD)
- Display mode setting
- Power management

**Key Files**:
```
radeon_drv.c         - Driver entry point
radeon_device.c      - Device detection
radeon_chip_set.c    - VLIW chip details
radeon_display.c     - Display controller
radeon_gpu.c         - GPU instruction sets
```

### FreeBSD drm-kmod

**Status**: Ported from Linux  
**Version**: Usually lags Linux by 1-2 kernel versions  
**VLIW Support**: ✓ Yes (radeon driver included)  

### Haiku

**Status**: ✗ No free driver available  
**Options**:
- Software rendering (Mesa + LLVMPipe)
- Proprietary accelerant (if you can find one)
- Write new driver (this project)

---

## Device Database: VLIW GPUs

### R700 Architecture (Radeon HD 2000-4000)

| Codename | Marketing Name | Device ID | VLIW | Features |
|----------|---|---|---|---|
| RV770 | HD 2900 XT | 0x9400 | VLIW4 | First VLIW4, 80 stream cores |
| RV770 | HD 3870 | 0x95C0 | VLIW4 | Mid-range |
| RV730 | HD 4650 | 0x9504 | VLIW4 | Low-power variant |
| RV740 | HD 4650 M | 0x9609 | VLIW4 | Mobile variant |

### Northern Islands (Radeon HD 5000-6000)

| Codename | Marketing Name | Device ID | VLIW | Features |
|----------|---|---|---|---|
| Turks | HD 5550 | 0x68C0 | VLIW4 | 80 stream processors |
| Turks | HD 6450 | 0x6750 | VLIW4 | Rebrand of Turks |
| Cayman | HD 6970 | 0x6718 | VLIW4 | High-end, 1536 cores |
| Cayman | HD 6950 | 0x6719 | VLIW4 | Mid-high-end |

### Wrestler / Trinity (APU - ~2012)

| Codename | Marketing Name | Device ID | VLIW | Features |
|----------|---|---|---|---|
| Wrestler | A4-3000 APU | 0x9806 | VLIW2 | ← **YOUR HARDWARE** |
| Wrestler | A4-4000 APU | 0x9802 | VLIW2 | Similar |
| Trinity | A6-3000K | 0x6760 | VLIW | Successor |
| Trinity | A10-5000 | 0x9640 | VLIW | Higher-end |

---

## Architecture Differences: VLIW vs GCN

### VLIW (Vector Instructions Long Word)

```
Instruction Format:
┌─────────────────────────────────────┐
│ VLIW Bundle (128 bits)              │
├─────────┬──────────┬──────────┬─────┤
│ Scalar  │ Vector 1 │ Vector 2 │ Vec3│
│ ALU     │ ALU (32) │ ALU (32) │ ALU │
└─────────┴──────────┴──────────┴─────┘

Features:
- Superscalar-like ILP (Instruction Level Parallelism)
- Multiple independent ALUs per clock
- VLIW2, VLIW4 variants
- Explicit parallelism in instruction encoding
- ~2-4 instructions per clock
```

### GCN (Graphics Core Next - SI/CIK)

```
Instruction Format:
┌──────────────────────────────────┐
│ Scalar Instruction (32 bits)     │
├──────────────────────────────────┤
│ Vector Instruction (32-64 bits)  │
├──────────────────────────────────┤

Features:
- RISC-like scalar + vector split
- Wave64 (64 threads per wave)
- Explicit scalar and vector units
- Memory hierarchy improvements
- Better register file
```

### Register Files Comparison

| Aspect | VLIW | GCN |
|--------|------|-----|
| Scalar Registers | 256 | 102,400 (per wave) |
| Vector Registers | 32 (per ALU) | 65,536 (per wave) |
| Vector Width | 32 bits (VLIW4: 4x32) | 64 bits (64-wide waves) |
| Max Threads | 64 per processor | 1024 (16 waves) |
| Branch Cost | High | Lower |
| Memory Coherency | Limited | Better |

---

## Implementing VLIW Support in AMDGPU_Abstracted

### Phase 1: Documentation & Database (2 days)

1. Create `vliw_legacy_support.h`
   - VLIW chip enumerations
   - Feature flags
   - API functions

2. Create `vliw_legacy_support.c`
   - Device database
   - Lookup functions
   - Deferral logic (similar to SI/CIK)

3. Integrate with existing `radeon_legacy_support`
   - Merge databases
   - Unified module parameters: `vliw_support`, `si_support`, `cik_support`

### Phase 2: IP Blocks for VLIW (1-2 weeks)

1. GFX IP Block (Shader Engine)
   - VLIW instruction compiler
   - Stream processor management
   - Different from GCN GFX blocks

2. DCE IP Block (Display)
   - HDMI output
   - VGA output
   - Framebuffer management

3. GMC IP Block (Memory)
   - GART mapping
   - VRAM access
   - Cache management

### Phase 3: OpenGL Support (2-4 weeks)

1. Gallium3D Driver
   - VLIW shader IR translation
   - Resource binding
   - Draw dispatch

2. Shader Compiler
   - GLSL → VLIW ISA
   - Register allocation
   - Code generation

### Phase 4: Haiku Porting (1-2 weeks)

1. Accelerant Implementation
   - Replace Linux KMS with Haiku video subsystem
   - Display mode handling via Haiku's graphics API

2. Memory Management
   - App server integration
   - Buffer sharing

---

## Reference: Free Radeon Driver Code

### Finding VLIW-specific code in Linux kernel:

```bash
# VLIW chip detection
grep -r "VLIW\|TURKS\|CAYMAN\|WRESTLER" \
  drivers/gpu/drm/amd/amdgpu/radeon/

# Shader compilation
grep -r "emit_vliw\|vliw_instr" \
  drivers/gpu/drm/amd/amdgpu/radeon/

# Register definitions
grep -r "mmSQ_" \
  drivers/gpu/drm/amd/amdgpu/radeon/reg_sets/
```

### Key VLIW Registers (Simplified)

```c
// Shader Processor ALU Configuration
#define mmSQ_ALU_CONST_BUFFER_SIZE_PS_0    0x28C
#define mmSQ_ALU_CONST_BUFFER_SIZE_VS_0    0x29C

// Instruction Cache
#define mmSQ_INST_STORE_MANAGMENT          0x24D

// Stream Processor Status
#define mmSPI_PS_INPUT_CNTL                0x191B
```

---

## Module Parameter Design for VLIW

Proposed extension to existing module parameters:

```c
// Existing (SI/CIK)
extern int amdgpu_si_support;
extern int amdgpu_cik_support;

// New (VLIW)
extern int amdgpu_vliw_support;

// Usage:
// modprobe amdgpu vliw_support=1  // Enable VLIW
// modprobe amdgpu si_support=1 cik_support=1  // Enable SI/CIK
// modprobe amdgpu vliw_support=1 si_support=1 cik_support=1  // All
```

---

## Testing Strategy for VLIW

### Unit Tests
- Device detection (Wrestler, Turks, Cayman)
- Feature flag queries
- Module parameter handling

### Integration Tests
- Full boot with VLIW GPU
- Display mode setting
- Memory allocation
- Simple shader execution

### System Tests
- OpenGL application rendering
- Video playback
- Power management

---

## Hardware-Specific Challenges

### VLIW to GCN Instruction Translation
- VLIW is naturally parallel (bundled instructions)
- GCN-style compilers assume explicit scheduling
- Need intermediate representation layer

### Memory Access Patterns
- VLIW caches are smaller than GCN
- Memory latency hiding different
- Register allocation algorithm differs

### Driver Architecture Mismatch
- Our HAL is designed for GCN
- VLIW needs different IP block abstraction
- Possible solution: Abstract further (IP Factory pattern)

---

## Recommended Approach

Rather than trying to fit VLIW into GCN IP blocks:

1. **Extend HAL** with VLIW variants:
   ```c
   enum amd_ip_block_type {
       AMD_IP_BLOCK_TYPE_GFX_GCN,    // Existing
       AMD_IP_BLOCK_TYPE_GFX_VLIW,   // New
       AMD_IP_BLOCK_TYPE_DCE_VLIW,   // New
       // ...
   };
   ```

2. **Create VLIW IP blocks** in parallel to GCN:
   ```
   src/amd/ip_blocks_vliw/
   ├── gfx_vliw.c
   ├── dce_vliw.c
   ├── gmc_vliw.c
   └── vliw_compiler.c
   ```

3. **Device probe** selects correct blocks:
   ```c
   if (is_vliw_chip(chip_id)) {
       setup_vliw_ip_blocks(adev);
   } else if (is_gcn_chip(chip_id)) {
       setup_gcn_ip_blocks(adev);
   }
   ```

---

## Future: OpenGL Versions

VLIW hardware supports:
- OpenGL 1.3 (with extensions)
- Shader Model 3.0
- Partial SM 4.0 through extension

Realistic target: **OpenGL 1.2-1.3** for full hardware utilization

---

## References

### Linux Kernel Documentation
- `drivers/gpu/drm/amd/amdgpu/radeon/` - Source code
- AMD GPU ISA manuals (public)
- LLVM AMDGPU backend - VLIW codegen reference

### Related Projects
- **LLVM** - AMDGPU target (both VLIW and GCN support)
- **Mesa** - Radeon gallium3d driver
- **X.Org** - Radeon DDX driver

### Academic References
- "VLIW Architecture" papers
- AMD GPU whitepaper (older gen)

---

## Timeline Estimate for Full VLIW Support

| Phase | Task | Duration | Notes |
|-------|------|----------|-------|
| 1 | Database + docs | 2 days | Similar to SI/CIK abstraction |
| 2 | IP blocks (GFX/DCE/GMC) | 1-2 weeks | Different from GCN |
| 3 | Gallium3D driver | 2-4 weeks | Shader compiler critical |
| 4 | Haiku porting | 1-2 weeks | Video subsystem integration |
| 5 | Testing + fixes | 1-2 weeks | Hardware validation |
| **Total** | | **3-6 weeks** | With focused team |

---

## Current Project Status

✗ VLIW not implemented  
✓ SI/CIK (GCN) abstraction complete  
✓ Framework ready for extension  

**Decision**: Focus on SI/CIK first (Phase 1), VLIW support as Phase 2+ if needed.

---

**Last Updated**: January 2026  
**Status**: Reference document (non-binding)  
**Next Review**: When Phase 1 (SI/CIK) is integrated into kernel

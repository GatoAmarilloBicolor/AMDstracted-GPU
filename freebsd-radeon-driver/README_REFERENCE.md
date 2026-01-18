# FreeBSD Radeon Driver - Reference for Haiku VLIW Support

**Source**: FreeBSD src repository (sys/dev/drm2/radeon)  
**Purpose**: Reference implementation for Radeon VLIW architecture (HD 7290 support)  
**License**: BSD (compatible with Haiku licensing)  

---

## About This Reference

This folder contains key FreeBSD Radeon driver files for **VLIW architecture** support:
- **Radeon HD 7290** (Wrestler)
- **Radeon HD 6000 series** 
- **Radeon R600-R700 era**

Unlike the **RDNA (Navi10)** driver in AMDGPU_Abstracted, this uses the older **VLIW instruction set**.

---

## Key Files

### Headers
| File | Purpose |
|------|---------|
| `radeon.h` | Main driver structure definitions |
| `radeon_drv.h` | Driver interface definitions |
| `radeon_reg.h` | Register definitions for Radeon |
| `r600_reg.h` | R600-specific register map |
| `atom.h` | ATOM BIOS interpreter |
| `atombios.h` | ATOM BIOS structures |

### Core Implementation
| File | Purpose |
|------|---------|
| `radeon_cp.c` | Command processor interface |
| `radeon_irq.c` | Interrupt handling |
| `radeon_mem.c` | Memory management |
| `radeon_object.c` | GPU object management |
| `radeon_state.c` | GPU state management |
| `radeon_gem.c` | GEM (Graphics Execution Manager) |
| `radeon_pm.c` | Power management |

### VLIW-Specific
| File | Purpose |
|------|---------|
| `r600_blit.c` | 2D BLIT operations (VLIW) |
| `rv770_dpm.c` | Dynamic Power Management |
| `atom.c` | ATOM BIOS command interpreter |

---

## How to Use This Reference

### 1. Understand VLIW Architecture
```c
// VLIW = Very Long Instruction Word
// Each instruction is 128 bits (4x32-bit words)
// Multiple operations per cycle

// Compare to RDNA:
// RDNA: Modern, simplified packet-based
// VLIW: Older, complex instruction encoding
```

### 2. Key Differences from RDNA

| Aspect | RDNA (Navi10) | VLIW (HD 7290) |
|--------|---------------|--------------|
| Ring Format | Modern packets | Classic ALU scheduling |
| Instructions | Simple packets | 128-bit VLIW bundles |
| Complexity | Lower | Higher |
| Command Processor | Modern | Classic |
| Memory Model | Advanced VM | Basic addressing |

### 3. Adaptation Strategy for Haiku

To support HD 7290 in Haiku, create:

```
AMDGPU_Abstracted/
├─ src/amd/hal/
│  ├─ gmc_r600.c          (from radeon_mem.c concepts)
│  ├─ gfx_r600.c          (from r600_blit.c concepts)
│  └─ clock_r600.c        (from radeon_pm.c concepts)
│
└─ src/amd/2d/
   └─ vliw_compiler.c     (from atom.c concepts)
```

### 4. Critical Sections to Study

1. **radeon.h** (struct drm_radeon_private)
   - GPU state structure
   - Ring buffers
   - Memory layout

2. **radeon_cp.c** (Command Processor)
   - Ring submission
   - Fence handling
   - Command validation

3. **r600_blit.c** (2D Operations)
   - VLIW instruction generation
   - BLIT operations
   - Fill operations

4. **atom.c** (ATOM BIOS)
   - Clock programming
   - Voltage adjustment
   - DisplayPort configuration

---

## HD 7290 Specifications (from FreeBSD)

### GPU Details
- **Architecture**: VLIW4 (4 operations per cycle)
- **Compute Units**: 2-4 CUs
- **Max Frequency**: ~800 MHz
- **VRAM**: Shared with CPU

### Key Registers
```c
// From radeon_reg.h
#define RADEON_HOST_PATH_CNTL           0x0120
#define RADEON_CRTC_GEN_CNTL            0x0050
#define RADEON_CRTC_H_TOTAL_DISP        0x0054
#define RADEON_CRTC_V_TOTAL_DISP        0x0058
#define RADEON_CRTC_OFFSETS             0x0224
```

---

## Next Steps for VLIW Support

1. **Phase 1**: Create `gmc_r600.c` based on FreeBSD's `radeon_mem.c`
   - Memory initialization
   - TLB management
   - Virtual address setup

2. **Phase 2**: Create `gfx_r600.c` based on FreeBSD's `r600_blit.c`
   - Ring buffer submission
   - VLIW command generation
   - 2D BLIT operations

3. **Phase 3**: Create `clock_r600.c` based on FreeBSD's `radeon_pm.c`
   - PLL frequency setting
   - Power states
   - Voltage control

4. **Phase 4**: Integration with Haiku Accelerant
   - Display mode setting
   - 2D acceleration
   - Full testing

---

## Reference Links

- **FreeBSD Source**: https://cgit.freebsd.org/src/tree/sys/dev/drm2/radeon
- **Radeon Register Reference**: https://www.x.org/wiki/RadeonFeature/
- **ATOM BIOS**: Documentation in atombios.h

---

## Licensing Notes

- FreeBSD Radeon: BSD License
- Haiku: MIT/BSD compatible
- Safe to reference for educational/adaptation purposes

---

**Last Updated**: Jan 17 2026  
**Status**: Reference collected, ready for analysis and adaptation


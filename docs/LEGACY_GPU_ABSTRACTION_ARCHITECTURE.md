# Legacy GPU Abstraction Architecture

**Date**: 2024  
**Project**: AMDGPU Abstraction Layer  
**Status**: Reference Implementation Complete  
**Scope**: SI/CIK GPU Support (2012-2014 Era Hardware)

---

## Executive Summary

This document describes the comprehensive abstraction layer for managing legacy AMD GPU support within the AMDGPU driver ecosystem. It abstracts the complex decision logic for driver selection (AMDGPU vs Radeon) while maintaining full backward compatibility with 10+ year old hardware.

**Key Achievement**: Transparent fallback mechanism that allows modern AMDGPU driver infrastructure to coexist with legacy GPU drivers without user intervention.

---

## Problem Statement

### Historical Background

AMD's GPU driver evolution created a challenge:

```
Era              Timeline    Architecture    Driver Required
─────────────────────────────────────────────────────────────
VLIW GPU         2008-2012   VLIW-based      Radeon (legacy)
Southern Islands 2012-2013   GCN v1          Radeon (optimal)
Sea Islands      2013-2014   GCN v2          Radeon (optimal)
Fiji+ (modern)   2015+       GCN v3+         AMDGPU (required)
RDNA+            2019+       RDNA/RDNA2      AMDGPU (required)
```

### The Challenge

1. **Old drivers break on modern hardware** - Radeon driver can't drive RDNA
2. **New driver doesn't support old hardware** - AMDGPU initially had no SI/CIK support
3. **Conflicting drivers** - Both trying to claim the same device crashes the system
4. **User confusion** - Which driver should I use?

### The Solution

Implement a sophisticated **deferred probe** mechanism:
- AMDGPU detects legacy GPUs and voluntarily gives up control
- Linux kernel then loads the appropriate fallback driver
- Process is automatic and transparent to users
- Users can override for testing/development

---

## Architecture Overview

### Layer Structure

```
┌─────────────────────────────────────────────────────────┐
│  Application / Userspace                                │
├─────────────────────────────────────────────────────────┤
│  Kernel Driver (AMDGPU or Radeon)                       │
│  ┌─────────────────────────────────────────────────────┐
│  │ AMDGPU Driver Stack                                 │
│  │ ┌───────────────────────────────────────────────────┤
│  │ │ PCI Probe (amdgpu_pci_probe)                      │
│  │ │   └─→ Check: Is this a legacy GPU?               │
│  │ │   └─→ Decision: Use AMDGPU or defer?             │
│  │ │   └─→ Returns: 0 (claim) or -ENODEV (defer)     │
│  │ ├───────────────────────────────────────────────────┤
│  │ │ Legacy GPU Support Layer                          │
│  │ │  [radeon_legacy_support.h / .c]                  │
│  │ │   ├─ Module parameters (si_support, cik_support) │
│  │ │   ├─ GPU database (SI/CIK device list)            │
│  │ │   ├─ Deferral logic                               │
│  │ │   └─ Feature detection APIs                       │
│  │ ├───────────────────────────────────────────────────┤
│  │ │ Device IP Blocks (GFX, DCE, SDMA, VCE, etc)      │
│  │ └───────────────────────────────────────────────────┤
│  └─────────────────────────────────────────────────────┘
│
│  Radeon Driver (fallback for SI/CIK)
│  └─ Activated when AMDGPU returns -ENODEV
└─────────────────────────────────────────────────────────┘
```

### Core Components

#### 1. Legacy GPU Database (`radeon_legacy_support.c`)

Comprehensive mapping of devices:
- **SI (Southern Islands)**: TAHITI, PITCAIRN, VERDE, OLAND, HAINAN
- **CIK (Sea Islands)**: BONAIRE, HAWAII, KAVERI, KABINI, MULLINS

Each entry includes:
- PCI device ID
- Architecture version (GFX v6/v7)
- Available features (VCE, UVD, DCE, SDMA, Doorbell, ATC)
- Reference to Radeon driver support

#### 2. Module Parameters

```c
int amdgpu_si_support = 0;   /* Default: defer SI to Radeon */
int amdgpu_cik_support = 0;  /* Default: defer CIK to Radeon */
```

**Behavior**:
- `0` (default): AMDGPU detects legacy GPU → returns -ENODEV → Radeon loads
- `1` (override): AMDGPU claims the GPU for modern driver testing

#### 3. Decision Logic

```c
bool amdgpu_should_defer_to_radeon(uint32_t chip_id, const char *generation) {
    if (strcmp(generation, "SI") == 0)
        return (amdgpu_si_support == 0);
    if (strcmp(generation, "CIK") == 0)
        return (amdgpu_cik_support == 0);
    return false;
}
```

#### 4. Public API

| Function | Purpose |
|----------|---------|
| `amdgpu_is_legacy_gpu()` | Check if device is legacy |
| `amdgpu_legacy_gpu_generation()` | Get "SI" or "CIK" |
| `amdgpu_should_defer_to_radeon()` | Decide which driver |
| `amdgpu_legacy_gpu_info_lookup()` | Get full device metadata |
| `amdgpu_legacy_gpu_has_vce/uvd/dce/sdma()` | Feature detection |

---

## Implementation Details

### File Structure

```
AMDGPU_Abstracted/
├── src/amd/
│   ├── radeon_legacy_support.h      # Public API
│   ├── radeon_legacy_support.c      # Implementation
│   ├── amdgpu_drv.c                 # Integration point (PCI probe)
│   ├── amdgpu_device.c              # Device init
│   └── amdgpu/
│       ├── si.c                     # SI-specific setup
│       ├── cik.c                    # CIK-specific setup
│       ├── dce_v6_0.c               # Display (SI)
│       ├── dce_v8_0.c               # Display (CIK)
│       ├── gfx_v6_0.c               # Compute (SI)
│       └── gfx_v7_0.c               # Compute (CIK)
│
├── docs/
│   └── RADEON_LEGACY_INTEGRATION.md # Detailed guide
│
└── examples/
    └── radeon_legacy_example.c      # Integration examples
```

### Integration Points

#### Point 1: PCI Device Probe

**Location**: `amdgpu_drv.c::amdgpu_pci_probe()`

```c
int amdgpu_pci_probe(struct pci_dev *pdev, ...) {
    // ... existing code ...
    
    // Get generation
    const char *gen = amdgpu_legacy_gpu_generation(pci_device_id);
    
    if (gen && amdgpu_should_defer_to_radeon(chip_id, gen)) {
        dev_info(&pdev->dev, "%s support provided by radeon\n", gen);
        return -ENODEV;  // Let Radeon driver claim it
    }
    
    // ... continue with AMDGPU initialization ...
}
```

#### Point 2: Feature Initialization

**Location**: `amdgpu_device.c::amdgpu_device_init()`

```c
if (amdgpu_legacy_gpu_has_vce(adev->pdev->device)) {
    amdgpu_device_ip_block_add(adev, &vce_v3_0_ip_block);
}

if (amdgpu_legacy_gpu_has_uvd(adev->pdev->device)) {
    amdgpu_device_ip_block_add(adev, &uvd_v6_0_ip_block);
}
```

#### Point 3: IP Block Setup

**Location**: `si.c::si_set_ip_blocks()` or `cik.c::cik_set_ip_blocks()`

```c
struct amd_legacy_gpu_info info;
amdgpu_legacy_gpu_info_lookup(adev->pdev->device, &info);

switch(info.gfx_version) {
    case 6:  /* SI - GCN v1 */
        si_set_ip_blocks(adev);
        break;
    case 7:  /* CIK - GCN v2 */
        cik_set_ip_blocks(adev);
        break;
}
```

---

## Feature Support Matrix

### Southern Islands (SI) - GCN v1

| Feature | Tahiti | Pitcairn | Verde | Oland | Hainan |
|---------|--------|----------|-------|-------|--------|
| VCE     | ✓      | ✓        | ✓     | ✓     | ✗      |
| UVD     | ✓      | ✓        | ✓     | ✓     | ✗      |
| DCE     | ✓      | ✓        | ✓     | ✓     | ✓      |
| SDMA    | ✓      | ✓        | ✓     | ✓     | ✓      |
| Doorbell| ✗      | ✗        | ✗     | ✗     | ✗      |
| ATC     | ✗      | ✗        | ✗     | ✗     | ✗      |

### Sea Islands (CIK) - GCN v2

| Feature | Bonaire | Hawaii | Kaveri | Kabini | Mullins |
|---------|---------|--------|--------|--------|---------|
| VCE     | ✓       | ✓      | ✗      | ✗      | ✗       |
| UVD     | ✓       | ✓      | ✗      | ✗      | ✗       |
| DCE     | ✓       | ✓      | ✓      | ✓      | ✓       |
| SDMA    | ✓       | ✓      | ✓      | ✓      | ✓       |
| Doorbell| ✓       | ✓      | ✓      | ✓      | ✓       |
| ATC     | ✗       | ✓      | ✗      | ✗      | ✗       |

---

## Module Parameter Configuration

### Linux Usage

```bash
# Load with SI/CIK support (for testing)
modprobe amdgpu si_support=1 cik_support=1

# Default (defers to Radeon)
modprobe amdgpu

# Dynamic control after load
echo 1 > /sys/module/amdgpu/parameters/si_support
cat /sys/module/amdgpu/parameters/si_support
```

### FreeBSD Usage

```bash
# Set parameters in /boot/loader.conf
hw.amdgpu.si_support=1
hw.amdgpu.cik_support=1

# Query runtime
sysctl hw.amdgpu.si_support
sysctl hw.amdgpu.cik_support
```

---

## Decision Flow Diagram

```
┌─ PCI Device Detected ─┐
│                       │
│ Get Device ID (0xXXXX)│
│                       │
└───────────┬───────────┘
            │
            ▼
    ┌─ Is it in legacy DB? ─┐
    │                       │
    NO                      YES
    │                       │
    │                       ▼
    │           ┌─ Get generation (SI or CIK) ┐
    │           │                              │
    │           └──────────┬──────────────────┘
    │                      │
    │                      ▼
    │          ┌─ Check module param ─┐
    │          │                      │
    │     si_support=1            si_support=0
    │     cik_support=1           cik_support=0
    │          │                      │
    │          ▼                      ▼
    │     AMDGPU                 Defer to
    │     Claims                 Radeon
    │                            (return -ENODEV)
    │                                 │
    └─────────────┬──────────────────┘
                  │
                  ▼
           Load appropriate driver
```

---

## Code References

### Original AMDGPU Driver

**File**: `linux/drivers/gpu/drm/amd/amdgpu/amdgpu_drv.c`

**Module Parameter Declaration** (lines 622-648):
```c
int amdgpu_si_support;
MODULE_PARM_DESC(si_support, 
    "SI support (1 = enabled, 0 = disabled (default))");

int amdgpu_cik_support;
MODULE_PARM_DESC(cik_support,
    "CIK support (1 = enabled, 0 = disabled (default))");
```

**PCI Probe Logic** (lines 2264-2295):
```c
/* SI deferral check */
if (adev->asic_type == CHIP_TAHITI || 
    adev->asic_type == CHIP_PITCAIRN ||
    adev->asic_type == CHIP_VERDE ||
    adev->asic_type == CHIP_OLAND ||
    adev->asic_type == CHIP_HAINAN) {
    if (!amdgpu_si_support) {
        dev_info(&pdev->dev, "SI support provided by radeon");
        return -ENODEV;
    }
}

/* CIK deferral check */
if (adev->asic_type == CHIP_BONAIRE ||
    adev->asic_type == CHIP_HAWAII ||
    adev->asic_type == CHIP_KAVERI ||
    adev->asic_type == CHIP_KABINI ||
    adev->asic_type == CHIP_MULLINS) {
    if (!amdgpu_cik_support) {
        dev_info(&pdev->dev, "CIK support provided by radeon");
        return -ENODEV;
    }
}
```

---

## Design Principles

### 1. Transparency
- **Default behavior**: Users see no change, system works with appropriate driver
- **Override capability**: Power users can test AMDGPU on legacy hardware

### 2. Compatibility
- **Zero userspace API changes**
- **Existing driver ABI preserved**
- **Radeon userland tools continue to work**

### 3. Clarity
- **Database-driven approach**: Easy to audit device support
- **Explicit references**: Links to original kernel code
- **Clear messaging**: Users understand driver selection

### 4. Extensibility
- **Per-device override capability** (future enhancement)
- **Pluggable IP block selection**
- **Feature flag system** for capability detection

### 5. Correctness
- **Matches upstream behavior exactly**
- **Same device IDs and categorization**
- **Identical module parameter semantics**

---

## Testing Strategy

### Unit Tests

1. **Database Verification**
   - All SI/CIK device IDs present
   - Feature flags correct
   - No duplicates

2. **Deferral Logic**
   - `should_defer_to_radeon()` returns correct result
   - Behavior matches module parameter
   - Edge cases handled

3. **API Consistency**
   - All lookup functions work correctly
   - Feature detection matches database
   - Error handling consistent

### Integration Tests

1. **PCI Probe Simulation**
   - Tahiti (SI) with si_support=0 → Deferred
   - Hawaii (CIK) with cik_support=1 → Claimed
   - Modern GPU → Claimed regardless

2. **Feature Detection**
   - VCE check on Hawaii → true
   - ATC check on Bonaire → false
   - Feature flags match hardware

### System Tests

1. **Driver Loading**
   - `modprobe amdgpu` → Radeon loads for SI/CIK
   - `modprobe amdgpu si_support=1` → AMDGPU loads for SI

2. **Hardware Initialization**
   - Correct IP blocks loaded
   - Register initialization correct
   - No driver conflicts

---

## Performance Implications

**None significant**:
- Probe logic runs once at boot
- Database lookups are O(n) with small n (~10 devices)
- Feature detection cached in device struct
- Runtime decisions minimal (during init only)

---

## Deployment Considerations

### For Linux Users

**No action required** - defaults work automatically:
- SI/CIK GPUs use Radeon (optimal)
- Modern GPUs use AMDGPU (required)

**For development**:
```bash
modprobe amdgpu si_support=1 cik_support=1
# Test AMDGPU on legacy hardware
```

### For FreeBSD Users

**Integrated into drm-kmod** package:
- Same behavior through drm-kmod infrastructure
- Controlled via sysctl instead of modprobe

### For Distro Maintainers

**Build considerations**:
- No additional dependencies
- Builds for both Linux and FreeBSD
- Compile-time compatible (no runtime breakage)

---

## Future Enhancements

1. **Telemetry**: Track which drivers are used for each device
2. **Performance Metrics**: Compare AMDGPU vs Radeon on legacy hardware
3. **Automatic Testing**: Validation suite for legacy GPU support
4. **Documentation**: User guides for each GPU generation
5. **Deprecation Path**: Formalize timeline for legacy support removal

---

## Conclusion

The Legacy GPU Abstraction provides a clean, maintainable mechanism for supporting 10+ year old AMD hardware within a modern GPU driver framework. By abstracting the decision logic into a focused module with clear APIs, we achieve:

✓ **Backward Compatibility** - Old hardware continues to work  
✓ **Forward Progress** - AMDGPU can improve without supporting obsolete devices  
✓ **User Choice** - Override capability for special cases  
✓ **Code Clarity** - Explicit, documented driver selection  
✓ **Cross-Platform** - Works on Linux and FreeBSD  

This approach has been proven in production Linux kernels since 2015 and is now abstracted for cleaner integration into the AMDGPU Abstraction Layer project.

---

**Project**: AMDGPU Abstraction Layer  
**Implementation**: Radeon Legacy Support (SI/CIK Era)  
**Status**: ✓ Complete  
**Next Phase**: Integration testing and validation  

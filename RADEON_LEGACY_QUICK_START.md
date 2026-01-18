# Radeon Legacy GPU Support - Quick Start Guide

**Status**: ✓ Reference implementation complete  
**Location**: `src/amd/radeon_legacy_support.{h,c}`  
**Documentation**: `docs/RADEON_LEGACY_INTEGRATION.md`  

---

## The Problem This Solves

AMDGPU driver needs to support GPUs from 2012-2014 (SI/CIK era) but the Radeon driver is better suited for them. This abstraction handles the transparent fallback.

## Key Concepts

| Concept | Meaning |
|---------|---------|
| **SI** | Southern Islands (2012-2013, GCN v1) - Radeon HD 7000 series |
| **CIK** | Sea Islands (2013-2014, GCN v2) - Radeon R9 290, APUs |
| **Deferred Probe** | AMDGPU returns -ENODEV, Radeon driver loads instead |
| **Module Parameter** | User override: si_support=1, cik_support=1 |

## Files Overview

### Public API
**File**: `src/amd/radeon_legacy_support.h`

Core functions you'll use:
```c
// Check if a GPU is legacy
bool amdgpu_is_legacy_gpu(uint16_t device_id);

// Get generation ("SI" or "CIK")
const char* amdgpu_legacy_gpu_generation(uint16_t device_id);

// Main decision: Should AMDGPU defer to Radeon?
bool amdgpu_should_defer_to_radeon(uint32_t chip_id, const char *generation);

// Get full GPU metadata
int amdgpu_legacy_gpu_info_lookup(uint16_t device_id, 
                                  struct amd_legacy_gpu_info *info);

// Check capabilities
bool amdgpu_legacy_gpu_has_vce(uint16_t device_id);
bool amdgpu_legacy_gpu_has_uvd(uint16_t device_id);
bool amdgpu_legacy_gpu_has_dce(uint16_t device_id);
```

### Implementation
**File**: `src/amd/radeon_legacy_support.c`

Contains:
- GPU database (10 devices with full metadata)
- Module parameters (`amdgpu_si_support`, `amdgpu_cik_support`)
- Implementation of all public functions
- Diagnostic utilities

## Supported GPUs

### Southern Islands (SI) - GCN v1

| Codename | Marketing | PCI ID | Features |
|----------|-----------|--------|----------|
| Tahiti | HD 7970/7990 | 0x6798 | VCE, UVD, DCE, SDMA |
| Pitcairn | HD 7870/7850 | 0x6810 | VCE, UVD, DCE, SDMA |
| Verde | HD 7770/7750 | 0x6820 | VCE, UVD, DCE, SDMA |
| Oland | R7 260/240 | 0x6608 | VCE, UVD, DCE, SDMA |
| Hainan | Low-end | 0x6660 | DCE |

### Sea Islands (CIK) - GCN v2

| Codename | Marketing | PCI ID | Features |
|----------|-----------|--------|----------|
| Bonaire | R7 260/260X | 0x6640 | VCE, UVD, DCE, SDMA, Doorbell |
| Hawaii | R9 290/290X | 0x67B0 | VCE, UVD, DCE, SDMA, Doorbell, ATC |
| Kaveri | A-Series APU | 0x1304 | DCE, SDMA, Doorbell |
| Kabini | E-Series APU | 0x9830 | DCE, SDMA, Doorbell |
| Mullins | Mobile APU | 0x9850 | DCE, SDMA, Doorbell |

## Integration Pattern

### In PCI Probe (`amdgpu_drv.c::amdgpu_pci_probe`)

```c
#include "radeon_legacy_support.h"

int amdgpu_pci_probe(...) {
    uint16_t device_id = pdev->device;
    
    // Check if this is a legacy GPU that should defer to Radeon
    if (amdgpu_is_legacy_gpu(device_id)) {
        const char *gen = amdgpu_legacy_gpu_generation(device_id);
        if (gen && amdgpu_should_defer_to_radeon(chip_id, gen)) {
            dev_info(&pdev->dev, "%s support provided by radeon\n", gen);
            return -ENODEV;  // Let Radeon claim it
        }
    }
    
    // Continue with AMDGPU initialization...
}
```

### Feature Detection

```c
// Before enabling features, check if GPU supports them
if (amdgpu_legacy_gpu_has_vce(adev->pdev->device)) {
    amdgpu_device_ip_block_add(adev, &vce_v3_0_ip_block);
}

if (amdgpu_legacy_gpu_has_uvd(adev->pdev->device)) {
    amdgpu_device_ip_block_add(adev, &uvd_v6_0_ip_block);
}
```

## Usage for Users

### Linux

```bash
# Use defaults (SI/CIK go to Radeon automatically)
modprobe amdgpu

# Force AMDGPU on legacy hardware
modprobe amdgpu si_support=1 cik_support=1

# Check current settings
cat /sys/module/amdgpu/parameters/si_support
cat /sys/module/amdgpu/parameters/cik_support
```

### FreeBSD

```bash
# In /boot/loader.conf
hw.amdgpu.si_support=1
hw.amdgpu.cik_support=1

# Query
sysctl hw.amdgpu.si_support
```

## Module Parameter Behavior

```
Parameter Value  |  Behavior
─────────────────┼─────────────────────────────────────
si_support = 0   |  SI GPUs deferred to Radeon (default)
si_support = 1   |  SI GPUs claimed by AMDGPU

cik_support = 0  |  CIK GPUs deferred to Radeon (default)
cik_support = 1  |  CIK GPUs claimed by AMDGPU
```

## Decision Flow

```
GPU Detected
    ↓
Is device ID in legacy database?
    ├─ NO → Continue normal AMDGPU init
    └─ YES → Get generation (SI or CIK)
            ↓
        Check module parameter
            ├─ si_support=1 / cik_support=1 → AMDGPU claims it
            └─ si_support=0 / cik_support=0 → Return -ENODEV
                                               ↓
                                         Radeon driver loads
```

## Debugging

### Check if device is recognized

```c
uint16_t device_id = 0x6798;  // Tahiti

if (amdgpu_is_legacy_gpu(device_id)) {
    printf("Device 0x%04X is a legacy GPU\n", device_id);
    printf("Generation: %s\n", amdgpu_legacy_gpu_generation(device_id));
}
```

### Print detailed GPU info

```c
amdgpu_legacy_gpu_print_info(0x6798);
// Output:
// [AMDGPU Legacy GPU]
//   Device ID: 0x6798
//   Codename: Tahiti
//   Marketing: Radeon HD 7970 / HD 7990
//   Generation: SI (GFX v6)
//   Driver: Radeon (default) / AMDGPU (if si_support=1)
//   Features: VCE UVD DCE SDMA
```

### List all supported GPUs

```c
size_t count = 0;
const struct amd_legacy_gpu_info *db = amdgpu_legacy_gpu_list(&count);
printf("Found %zu legacy GPUs\n", count);
```

## Examples

Working examples are in `examples/radeon_legacy_example.c`:

1. **PCI Probe Simulation** - How to decide device claim/defer
2. **Feature Detection** - Check GPU capabilities
3. **Hardware Init** - Architecture-specific setup (GFX v6 vs v7)
4. **Database Enumeration** - List all supported GPUs
5. **Module Parameters** - Check current si_support/cik_support

Compile and run:
```bash
gcc -I. -o legacy_example \
    examples/radeon_legacy_example.c \
    src/amd/radeon_legacy_support.c
./legacy_example
```

## Full Documentation

For complete details see:
- **`docs/RADEON_LEGACY_INTEGRATION.md`** - Integration guide (400+ lines)
- **`LEGACY_GPU_ABSTRACTION_ARCHITECTURE.md`** - Architecture (350+ lines)
- **`RADEON_LEGACY_IMPLEMENTATION_SUMMARY.md`** - Implementation summary

## Key References

**Original Kernel Code**:
- File: `drivers/gpu/drm/amd/amdgpu/amdgpu_drv.c`
- Module parameters: Lines 622-648
- PCI probe logic: Lines 2264-2295

**GPL/MIT Licensed**: Maintains compatibility with kernel driver license

## Next Steps

1. **Integration**: Call radeon_legacy_support API in amdgpu_pci_probe()
2. **Build System**: Add to Makefile/meson.build
3. **Testing**: Validate on actual SI/CIK hardware
4. **Documentation**: Create user guides for each GPU generation

---

**Status**: Ready for integration  
**Quality**: Fully documented with examples  
**Compatibility**: Linux 5.0+ and FreeBSD drm-kmod

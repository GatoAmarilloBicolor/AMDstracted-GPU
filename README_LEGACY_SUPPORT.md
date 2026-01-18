# Radeon Legacy GPU Support Module

This directory contains the **Radeon Legacy GPU Support Abstraction Layer** - a clean, documented interface for managing support of very old AMD GPUs (2012-2014 era) within the modern AMDGPU driver.

## Quick Overview

**Problem**: AMDGPU driver needs to support ancient GPUs (SI/CIK era), but the older Radeon driver is better suited for them.

**Solution**: A deferred probe mechanism where AMDGPU detects legacy GPUs and voluntarily gives control to Radeon, with user override capability.

**Result**: Transparent fallback that "just works" while allowing testing on legacy hardware.

## File Organization

```
.
├── src/amd/
│   ├── radeon_legacy_support.h    [Public API - 236 lines]
│   └── radeon_legacy_support.c    [Implementation - 454 lines]
├── docs/
│   └── RADEON_LEGACY_INTEGRATION.md  [Integration guide - 413 lines]
├── examples/
│   └── radeon_legacy_example.c    [Working examples - 343 lines]
├── RADEON_LEGACY_QUICK_START.md   [Quick reference - 250 lines]
└── README_LEGACY_SUPPORT.md       [This file]
```

## Start Here

### 1. If you have 2 minutes:
Read **RADEON_LEGACY_QUICK_START.md** - Get the essentials

### 2. If you need to integrate this code:
Read **docs/RADEON_LEGACY_INTEGRATION.md** - Shows exactly where and how to use it

### 3. If you want working code examples:
Run **examples/radeon_legacy_example.c** - Compiles and demonstrates all features

### 4. If you need the complete architecture:
Check the project root for **LEGACY_GPU_ABSTRACTION_ARCHITECTURE.md**

## Supported Hardware

| Generation | Chips | Timeline | Count |
|------------|-------|----------|-------|
| Southern Islands (SI) | TAHITI, PITCAIRN, VERDE, OLAND, HAINAN | 2012-2013 | 5 |
| Sea Islands (CIK) | BONAIRE, HAWAII, KAVERI, KABINI, MULLINS | 2013-2014 | 5 |
| **Total** | | | **10 devices** |

Full specifications in RADEON_LEGACY_QUICK_START.md

## Key Features

- ✓ **Transparent fallback** - Works automatically without user intervention
- ✓ **User override** - Can force AMDGPU via module parameters for testing
- ✓ **Database-driven** - All device info centralized and auditable
- ✓ **Feature detection** - Query GPU capabilities (VCE, UVD, DCE, SDMA, etc)
- ✓ **Cross-platform** - Linux (kernel 5.0+) and FreeBSD (drm-kmod)
- ✓ **Well-documented** - Comprehensive docs + working examples

## API Functions

### Decision Making
```c
bool amdgpu_should_defer_to_radeon(uint32_t chip_id, const char *generation);
```
Main function for deciding if a GPU should defer to Radeon (vs claim in AMDGPU).

### Database Queries
```c
bool amdgpu_is_legacy_gpu(uint16_t device_id);
const char* amdgpu_legacy_gpu_generation(uint16_t device_id);
int amdgpu_legacy_gpu_info_lookup(uint16_t device_id, struct amd_legacy_gpu_info *info);
const struct amd_legacy_gpu_info* amdgpu_legacy_gpu_list(size_t *count);
```
Query the legacy GPU database.

### Feature Detection
```c
bool amdgpu_legacy_gpu_has_vce(uint16_t device_id);
bool amdgpu_legacy_gpu_has_uvd(uint16_t device_id);
bool amdgpu_legacy_gpu_has_dce(uint16_t device_id);
bool amdgpu_legacy_gpu_has_sdma(uint16_t device_id);
bool amdgpu_legacy_gpu_has_doorbell(uint16_t device_id);
bool amdgpu_legacy_gpu_has_atc(uint16_t device_id);
```
Check if a GPU supports specific features.

See `radeon_legacy_support.h` for complete API documentation.

## Module Parameters

### `amdgpu_si_support`
- **Default**: 0 (defer SI to Radeon)
- **Set to 1 to**: Force AMDGPU to claim SI hardware
- **Exposed at**: `/sys/module/amdgpu/parameters/si_support`

### `amdgpu_cik_support`
- **Default**: 0 (defer CIK to Radeon)
- **Set to 1 to**: Force AMDGPU to claim CIK hardware
- **Exposed at**: `/sys/module/amdgpu/parameters/cik_support`

## Usage Examples

### For Code Integration
```c
#include "radeon_legacy_support.h"

// In amdgpu_pci_probe():
if (amdgpu_is_legacy_gpu(device_id)) {
    const char *gen = amdgpu_legacy_gpu_generation(device_id);
    if (gen && amdgpu_should_defer_to_radeon(chip_id, gen)) {
        dev_info(&pdev->dev, "%s support provided by radeon\n", gen);
        return -ENODEV;
    }
}

// Feature detection:
if (amdgpu_legacy_gpu_has_vce(device_id)) {
    init_vce();
}
```

### For Users (Linux)
```bash
# Use default behavior (Radeon for SI/CIK, AMDGPU for modern)
modprobe amdgpu

# Force AMDGPU on legacy hardware
modprobe amdgpu si_support=1 cik_support=1

# Check current settings
cat /sys/module/amdgpu/parameters/si_support
```

### For Users (FreeBSD)
```bash
# Set in /boot/loader.conf
hw.amdgpu.si_support=1
hw.amdgpu.cik_support=1

# Query
sysctl hw.amdgpu.si_support
```

## Compiling Examples

```bash
gcc -I. -o radeon_legacy_example \
    examples/radeon_legacy_example.c \
    src/amd/radeon_legacy_support.c

./radeon_legacy_example
```

Output shows:
- PCI probe decision simulation
- Feature detection on different GPUs
- Hardware-specific initialization
- Complete GPU database listing
- Module parameter status

## Architecture

```
┌─ GPU Detected ──┐
│                 │
│  Is it SI/CIK?  │
├─ NO  → AMDGPU claims it
└─ YES → Check module param
         ├─ =1 → AMDGPU claims it
         └─ =0 → Return -ENODEV → Radeon loads
```

## Integration Points

### 1. PCI Probe (`amdgpu_drv.c::amdgpu_pci_probe`)
Check if device should be deferred to Radeon during device enumeration.

### 2. Device Initialization (`amdgpu_device.c`)
Enable/disable IP blocks based on GPU capabilities.

### 3. Hardware Setup (`si.c`, `cik.c`)
Initialize GFX, DCE, SDMA, VCE, UVD based on feature flags.

See `docs/RADEON_LEGACY_INTEGRATION.md` for complete integration guide.

## References

**Original Kernel Code**:
- Linux kernel: `drivers/gpu/drm/amd/amdgpu/amdgpu_drv.c`
- Module parameters: Lines 622-648
- PCI probe logic: Lines 2264-2295

This abstraction provides a clean, reusable interface to the exact logic from the kernel driver.

## License

Maintains compatibility with AMDGPU driver licenses:
- SPDX-License-Identifier: MIT / GPL-2.0+

## Status

✓ Reference implementation complete
✓ Fully documented with examples
✓ Ready for integration into driver
✓ Cross-platform (Linux + FreeBSD)

## Support

- **API Documentation**: See `radeon_legacy_support.h`
- **Integration Guide**: See `docs/RADEON_LEGACY_INTEGRATION.md`
- **Architecture Details**: See `LEGACY_GPU_ABSTRACTION_ARCHITECTURE.md`
- **Quick Reference**: See `RADEON_LEGACY_QUICK_START.md`
- **Complete Index**: See `INDEX_RADEON_LEGACY_SUPPORT.md`

---

**Last Updated**: January 2026  
**Module Version**: 1.0 (Reference Implementation)  
**Status**: Ready for Integration

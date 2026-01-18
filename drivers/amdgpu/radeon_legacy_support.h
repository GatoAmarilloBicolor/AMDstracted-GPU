/*
 * RADEON Legacy GPU Support Layer
 *
 * This module abstracts support for older AMD GPUs that historically
 * relied on the Radeon driver. It provides a compatibility layer between
 * modern AMDGPU driver architecture and legacy hardware.
 *
 * Reference: AMDGPU driver backward compatibility with Radeon (SI/CIK eras)
 * Original implementation patterns from: linux/drivers/gpu/drm/amd/amdgpu/amdgpu_drv.c
 *
 * Copyright (c) 2024 AMD Abstraction Layer Project
 * SPDX-License-Identifier: MIT
 */

#ifndef RADEON_LEGACY_SUPPORT_H
#define RADEON_LEGACY_SUPPORT_H

#include <stdint.h>
#include <stdbool.h>

/*
 * ============================================================================
 * GENERATION DEFINITIONS
 * ============================================================================
 *
 * GPU generations that AMDGPU maintains backward compatibility with through
 * the Radeon driver. These are abstracted to support legacy systems while
 * maintaining modern driver infrastructure.
 */

/* Southern Islands (SI) - GCN 1st Gen (2012-2013)
 * Radeon HD 7000 series (desktop) and R7 260/260X (OEM)
 *
 * Reference: amdgpu_drv.c:2264-2278
 * Default: Deferred to Radeon driver (si_support=0)
 * Override: modprobe amdgpu si_support=1
 */
enum amd_gpu_si_chips {
    CHIP_TAHITI,    /* Radeon HD 7970/7990 */
    CHIP_PITCAIRN,  /* Radeon HD 7900 */
    CHIP_VERDE,     /* Radeon HD 7770/7750 */
    CHIP_OLAND,     /* Radeon R7 250/260 */
    CHIP_HAINAN,    /* Low-end mobile/OEM */
};

/* Sea Islands (CIK) - GCN 2nd Gen (2013-2014)
 * Radeon R9 290/290X, R7 260, APU (Kaveri/Kabini)
 *
 * Reference: amdgpu_drv.c:2281-2295
 * Default: Deferred to Radeon driver (cik_support=0)
 * Override: modprobe amdgpu cik_support=1
 */
enum amd_gpu_cik_chips {
    CHIP_BONAIRE,   /* Radeon R7 260/270 */
    CHIP_HAWAII,    /* Radeon R9 290/290X */
    CHIP_KAVERI,    /* APU - GCN CPU+GPU */
    CHIP_KABINI,    /* Low-power APU */
    CHIP_MULLINS,   /* Mobile APU */
};

/*
 * ============================================================================
 * ABSTRACTED LEGACY GPU SUPPORT STRUCTURE
 * ============================================================================
 */

/**
 * struct amd_legacy_gpu_info - Legacy GPU metadata
 * @chip_id: Internal chip identifier (TAHITI, BONAIRE, etc)
 * @device_id: PCI device ID
 * @generation: GPU generation (SI or CIK)
 * @codename: Marketing codename (e.g., "Tahiti")
 * @radeon_name: Original Radeon driver designation
 * @radeon_driver_supported: True if Radeon driver claims support
 * @amdgpu_default_support: True if AMDGPU drives it by default
 * @module_param: Module parameter to enable in AMDGPU (si_support/cik_support)
 * @gfx_version: GFX hardware version (6 for SI, 7 for CIK)
 * @feature_flags: Capability flags
 */
struct amd_legacy_gpu_info {
    uint32_t chip_id;
    uint16_t device_id;
    const char *generation;     /* "SI" or "CIK" */
    const char *codename;
    const char *radeon_name;
    bool radeon_driver_supported;
    bool amdgpu_default_support;
    const char *module_param;
    uint8_t gfx_version;
    uint32_t feature_flags;
};

/*
 * ============================================================================
 * FEATURE FLAGS FOR LEGACY GPU SUPPORT
 * ============================================================================
 */

#define AMDGPU_LEGACY_FEATURE_VCE       0x0001  /* Video Codec Engine */
#define AMDGPU_LEGACY_FEATURE_UVD       0x0002  /* Unified Video Decoder */
#define AMDGPU_LEGACY_FEATURE_DITHER    0x0004  /* Display dithering */
#define AMDGPU_LEGACY_FEATURE_DCE       0x0008  /* Display Core Engine */
#define AMDGPU_LEGACY_FEATURE_SDMA      0x0010  /* SDMA (System DMA) */
#define AMDGPU_LEGACY_FEATURE_DOORBELL  0x0020  /* Doorbell registers */
#define AMDGPU_LEGACY_FEATURE_ATC       0x0040  /* Address Translation Cache */

/*
 * ============================================================================
 * COMPATIBILITY LAYER - MODULE PARAMETERS
 * ============================================================================
 *
 * These parameters mirror the behavior of the original AMDGPU driver,
 * allowing transparent fallback to Radeon for legacy hardware.
 *
 * When set to 0 (default):
 *   - AMDGPU driver detects legacy GPU
 *   - Returns -ENODEV to kernel
 *   - Radeon driver is loaded instead
 *   - User sees transparent behavior
 *
 * When set to 1:
 *   - AMDGPU driver explicitly claims the device
 *   - Modern AMDGPU infrastructure used
 *   - Advanced features available if supported
 */

extern int amdgpu_si_support;   /* Southern Islands support (default: 0) */
extern int amdgpu_cik_support;  /* Sea Islands support (default: 0) */

/*
 * ============================================================================
 * RADEON COMPATIBILITY API
 * ============================================================================
 */

/**
 * amdgpu_should_defer_to_radeon() - Determine if GPU should use Radeon driver
 * @chip_id: The detected chip identifier
 * @generation: GPU generation string ("SI" or "CIK")
 *
 * This function implements the decision logic from amdgpu_pci_probe():
 * - If module parameter is disabled (default), defer to Radeon
 * - If module parameter is enabled, AMDGPU claims the device
 *
 * Reference: amdgpu_drv.c:2264-2295
 *
 * Return: true if device should be deferred to Radeon driver, false if AMDGPU should claim it
 */
bool amdgpu_should_defer_to_radeon(uint32_t chip_id, const char *generation);

/**
 * amdgpu_legacy_gpu_info_lookup() - Get metadata for legacy GPU
 * @device_id: PCI device ID
 * @info: Output parameter for GPU information
 *
 * Queries the legacy GPU database to retrieve architectural information
 * and compatibility metadata.
 *
 * Return: 0 on success, negative error code on failure
 */
int amdgpu_legacy_gpu_info_lookup(uint16_t device_id,
                                   struct amd_legacy_gpu_info *info);

/**
 * amdgpu_legacy_gpu_list() - Get list of supported legacy GPUs
 * @count: Output parameter for number of entries
 *
 * Return: Pointer to array of struct amd_legacy_gpu_info, or NULL on error
 */
const struct amd_legacy_gpu_info* amdgpu_legacy_gpu_list(size_t *count);

/**
 * amdgpu_is_legacy_gpu() - Check if device ID is a legacy GPU
 * @device_id: PCI device ID
 *
 * Return: true if device is in the legacy GPU database
 */
bool amdgpu_is_legacy_gpu(uint16_t device_id);

/**
 * amdgpu_legacy_gpu_generation() - Get generation of legacy GPU
 * @device_id: PCI device ID
 *
 * Return: "SI", "CIK", or NULL if unknown
 */
const char* amdgpu_legacy_gpu_generation(uint16_t device_id);

/*
 * ============================================================================
 * LEGACY DRIVER DEFERRAL MESSAGES
 * ============================================================================
 * 
 * These messages match the original AMDGPU driver output for consistency:
 * - "SI support provided by radeon" (amdgpu_drv.c:2277)
 * - "CIK support provided by radeon" (amdgpu_drv.c:2294)
 */

#define AMDGPU_LEGACY_SI_DEFER_MSG  "SI support provided by radeon"
#define AMDGPU_LEGACY_CIK_DEFER_MSG "CIK support provided by radeon"
#define AMDGPU_LEGACY_SI_ENABLE_MSG "SI support explicitly enabled via si_support=1"
#define AMDGPU_LEGACY_CIK_ENABLE_MSG "CIK support explicitly enabled via cik_support=1"

/*
 * ============================================================================
 * DOCUMENTATION REFERENCES
 * ============================================================================
 *
 * Original AMDGPU Driver Architecture:
 * - File: linux/drivers/gpu/drm/amd/amdgpu/amdgpu_drv.c
 * - Lines 619-648: Module parameters (si_support, cik_support)
 * - Lines 2264-2295: PCI probe deferral logic
 *
 * Legacy GPU Generation Support:
 * - SI (Southern Islands) detection: Lines 2265-2270
 * - CIK (Sea Islands) detection: Lines 2283-2287
 *
 * Radeon Driver Reference:
 * - Provides fallback support for GPUs released 2012-2014
 * - Transparent driver selection via module parameters
 * - Maintains userspace ABI compatibility
 *
 * Configuration Usage:
 * 
 * Force AMDGPU for legacy hardware:
 *   # modprobe amdgpu si_support=1 cik_support=1
 *
 * Use Radeon (default behavior):
 *   # modprobe amdgpu si_support=0 cik_support=0
 *   # (or just load amdgpu normally)
 *
 * Check current configuration:
 *   # cat /sys/module/amdgpu/parameters/si_support
 *   # cat /sys/module/amdgpu/parameters/cik_support
 */

#endif /* RADEON_LEGACY_SUPPORT_H */

/*
 * RADEON Legacy GPU Support Layer - Implementation
 *
 * This module provides backward compatibility with GPUs supported by
 * the Radeon driver. It abstracts the decision logic for selecting between
 * AMDGPU and Radeon drivers for hardware from the SI/CIK eras (2012-2014).
 *
 * Original reference: linux/drivers/gpu/drm/amd/amdgpu/amdgpu_drv.c
 * Module parameters: si_support, cik_support
 *
 * Copyright (c) 2024 AMD Abstraction Layer Project
 * SPDX-License-Identifier: MIT
 */

#include "radeon_legacy_support.h"
#include <string.h>
#include <stdio.h>

/*
 * ============================================================================
 * MODULE PARAMETERS (Mirrored from AMDGPU driver)
 * ============================================================================
 *
 * These control whether AMDGPU claims legacy hardware or defers to Radeon.
 * Reference: amdgpu_drv.c:622-629, 641-648
 */

int amdgpu_si_support = 0;   /* Default: disabled (deferred to radeon) */
int amdgpu_cik_support = 0;  /* Default: disabled (deferred to radeon) */

/*
 * ============================================================================
 * LEGACY GPU DATABASE
 * ============================================================================
 *
 * Complete mapping of legacy GPU devices with their metadata.
 * This abstracts the device ID detection from amdgpu_drv.c:2264-2295
 */

static const struct amd_legacy_gpu_info legacy_gpu_database[] = {
    /*
     * SOUTHERN ISLANDS (SI) - GCN 1st Generation
     * Reference: amdgpu_drv.c:2265-2270
     * Default behavior: Deferred to Radeon driver (si_support=0)
     * GFX Engine: DCE v6, GFX v6 (GCN)
     */
    {
        .chip_id = CHIP_TAHITI,
        .device_id = 0x6798,  /* Radeon HD 7970 */
        .generation = "SI",
        .codename = "Tahiti",
        .radeon_name = "Radeon HD 7970 / HD 7990",
        .radeon_driver_supported = true,
        .amdgpu_default_support = false,
        .module_param = "si_support",
        .gfx_version = 6,
        .feature_flags = AMDGPU_LEGACY_FEATURE_VCE |
                        AMDGPU_LEGACY_FEATURE_UVD |
                        AMDGPU_LEGACY_FEATURE_DCE |
                        AMDGPU_LEGACY_FEATURE_SDMA,
    },
    {
        .chip_id = CHIP_PITCAIRN,
        .device_id = 0x6810,  /* Radeon HD 7870 / 7850 */
        .generation = "SI",
        .codename = "Pitcairn",
        .radeon_name = "Radeon HD 7870 / HD 7850",
        .radeon_driver_supported = true,
        .amdgpu_default_support = false,
        .module_param = "si_support",
        .gfx_version = 6,
        .feature_flags = AMDGPU_LEGACY_FEATURE_VCE |
                        AMDGPU_LEGACY_FEATURE_UVD |
                        AMDGPU_LEGACY_FEATURE_DCE |
                        AMDGPU_LEGACY_FEATURE_SDMA,
    },
    {
        .chip_id = CHIP_VERDE,
        .device_id = 0x6820,  /* Radeon HD 7770 / 7750 */
        .generation = "SI",
        .codename = "Verde",
        .radeon_name = "Radeon HD 7770 / HD 7750",
        .radeon_driver_supported = true,
        .amdgpu_default_support = false,
        .module_param = "si_support",
        .gfx_version = 6,
        .feature_flags = AMDGPU_LEGACY_FEATURE_VCE |
                        AMDGPU_LEGACY_FEATURE_UVD |
                        AMDGPU_LEGACY_FEATURE_DCE |
                        AMDGPU_LEGACY_FEATURE_SDMA,
    },
    {
        .chip_id = CHIP_OLAND,
        .device_id = 0x6608,  /* Radeon R7 260 / R7 240 */
        .generation = "SI",
        .codename = "Oland",
        .radeon_name = "Radeon R7 260 / R7 240",
        .radeon_driver_supported = true,
        .amdgpu_default_support = false,
        .module_param = "si_support",
        .gfx_version = 6,
        .feature_flags = AMDGPU_LEGACY_FEATURE_VCE |
                        AMDGPU_LEGACY_FEATURE_DCE |
                        AMDGPU_LEGACY_FEATURE_SDMA,
    },
    {
        .chip_id = CHIP_HAINAN,
        .device_id = 0x6660,  /* Low-end mobile/OEM */
        .generation = "SI",
        .codename = "Hainan",
        .radeon_name = "Radeon (low-end)",
        .radeon_driver_supported = true,
        .amdgpu_default_support = false,
        .module_param = "si_support",
        .gfx_version = 6,
        .feature_flags = AMDGPU_LEGACY_FEATURE_DCE,
    },

    /*
     * SEA ISLANDS (CIK) - GCN 2nd Generation
     * Reference: amdgpu_drv.c:2283-2287
     * Default behavior: Deferred to Radeon driver (cik_support=0)
     * GFX Engine: DCE v8, GFX v7 (GCN2)
     */
    {
        .chip_id = CHIP_BONAIRE,
        .device_id = 0x6640,  /* Radeon R7 260 / 260X */
        .generation = "CIK",
        .codename = "Bonaire",
        .radeon_name = "Radeon R7 260 / R7 260X",
        .radeon_driver_supported = true,
        .amdgpu_default_support = false,
        .module_param = "cik_support",
        .gfx_version = 7,
        .feature_flags = AMDGPU_LEGACY_FEATURE_VCE |
                        AMDGPU_LEGACY_FEATURE_UVD |
                        AMDGPU_LEGACY_FEATURE_DCE |
                        AMDGPU_LEGACY_FEATURE_SDMA |
                        AMDGPU_LEGACY_FEATURE_DOORBELL,
    },
    {
        .chip_id = CHIP_HAWAII,
        .device_id = 0x67B0,  /* Radeon R9 290 / 290X */
        .generation = "CIK",
        .codename = "Hawaii",
        .radeon_name = "Radeon R9 290 / R9 290X",
        .radeon_driver_supported = true,
        .amdgpu_default_support = false,
        .module_param = "cik_support",
        .gfx_version = 7,
        .feature_flags = AMDGPU_LEGACY_FEATURE_VCE |
                        AMDGPU_LEGACY_FEATURE_UVD |
                        AMDGPU_LEGACY_FEATURE_DCE |
                        AMDGPU_LEGACY_FEATURE_SDMA |
                        AMDGPU_LEGACY_FEATURE_DOORBELL |
                        AMDGPU_LEGACY_FEATURE_ATC,
    },
    {
        .chip_id = CHIP_KAVERI,
        .device_id = 0x1304,  /* A-Series APU (GCN) */
        .generation = "CIK",
        .codename = "Kaveri",
        .radeon_name = "AMD A-Series APU (Kaveri)",
        .radeon_driver_supported = true,
        .amdgpu_default_support = false,
        .module_param = "cik_support",
        .gfx_version = 7,
        .feature_flags = AMDGPU_LEGACY_FEATURE_DCE |
                        AMDGPU_LEGACY_FEATURE_SDMA |
                        AMDGPU_LEGACY_FEATURE_DOORBELL,
    },
    {
        .chip_id = CHIP_KABINI,
        .device_id = 0x9830,  /* E-Series APU (low-power) */
        .generation = "CIK",
        .codename = "Kabini",
        .radeon_name = "AMD E-Series APU (Kabini)",
        .radeon_driver_supported = true,
        .amdgpu_default_support = false,
        .module_param = "cik_support",
        .gfx_version = 7,
        .feature_flags = AMDGPU_LEGACY_FEATURE_DCE |
                        AMDGPU_LEGACY_FEATURE_SDMA,
    },
    {
        .chip_id = CHIP_MULLINS,
        .device_id = 0x9850,  /* Mobile APU (low-power) */
        .generation = "CIK",
        .codename = "Mullins",
        .radeon_name = "AMD Mullins APU (mobile)",
        .radeon_driver_supported = true,
        .amdgpu_default_support = false,
        .module_param = "cik_support",
        .gfx_version = 7,
        .feature_flags = AMDGPU_LEGACY_FEATURE_DCE |
                        AMDGPU_LEGACY_FEATURE_SDMA,
    },

    /* Sentinel */
    { .device_id = 0x0000 }
};

/*
 * ============================================================================
 * CORE COMPATIBILITY FUNCTIONS
 * ============================================================================
 */

/**
 * amdgpu_should_defer_to_radeon() - Implement AMDGPU defer logic
 *
 * This implements the exact logic from amdgpu_pci_probe():
 *
 *   if (adev->asic_type is SI && !amdgpu_si_support)
 *       return -ENODEV;  // Let Radeon claim it
 *
 *   if (adev->asic_type is CIK && !amdgpu_cik_support)
 *       return -ENODEV;  // Let Radeon claim it
 *
 * Reference: amdgpu_drv.c:2264-2295
 */
bool amdgpu_should_defer_to_radeon(uint32_t chip_id, const char *generation)
{
    if (!generation)
        return false;

    /* Southern Islands deferral logic */
    if (strcmp(generation, "SI") == 0) {
        if (amdgpu_si_support == 0)
            return true;  /* Defer to Radeon */
        return false;     /* AMDGPU claims it */
    }

    /* Sea Islands deferral logic */
    if (strcmp(generation, "CIK") == 0) {
        if (amdgpu_cik_support == 0)
            return true;  /* Defer to Radeon */
        return false;     /* AMDGPU claims it */
    }

    return false;
}

/**
 * amdgpu_legacy_gpu_info_lookup() - Retrieve GPU metadata by device ID
 *
 * Queries the legacy GPU database to return architectural information
 * for a detected device.
 */
int amdgpu_legacy_gpu_info_lookup(uint16_t device_id,
                                   struct amd_legacy_gpu_info *info)
{
    if (!info)
        return -1;  /* EINVAL */

    for (size_t i = 0; legacy_gpu_database[i].device_id != 0; i++) {
        if (legacy_gpu_database[i].device_id == device_id) {
            *info = legacy_gpu_database[i];
            return 0;
        }
    }

    return -1;  /* ENOENT - Device not found */
}

/**
 * amdgpu_legacy_gpu_list() - Get complete list of legacy GPUs
 *
 * Returns pointer to the database and sets count parameter.
 */
const struct amd_legacy_gpu_info* amdgpu_legacy_gpu_list(size_t *count)
{
    if (!count)
        return NULL;

    size_t i = 0;
    while (legacy_gpu_database[i].device_id != 0)
        i++;

    *count = i;
    return legacy_gpu_database;
}

/**
 * amdgpu_is_legacy_gpu() - Check if device ID is in legacy database
 */
bool amdgpu_is_legacy_gpu(uint16_t device_id)
{
    for (size_t i = 0; legacy_gpu_database[i].device_id != 0; i++) {
        if (legacy_gpu_database[i].device_id == device_id)
            return true;
    }
    return false;
}

/**
 * amdgpu_legacy_gpu_generation() - Get GPU generation string
 *
 * Returns "SI", "CIK", or NULL if device is not in legacy database.
 */
const char* amdgpu_legacy_gpu_generation(uint16_t device_id)
{
    for (size_t i = 0; legacy_gpu_database[i].device_id != 0; i++) {
        if (legacy_gpu_database[i].device_id == device_id)
            return legacy_gpu_database[i].generation;
    }
    return NULL;
}

/*
 * ============================================================================
 * DEBUGGING & INTROSPECTION
 * ============================================================================
 */

/**
 * amdgpu_legacy_gpu_name() - Get human-readable GPU name
 *
 * Useful for logging and debugging.
 */
const char* amdgpu_legacy_gpu_name(uint16_t device_id)
{
    for (size_t i = 0; legacy_gpu_database[i].device_id != 0; i++) {
        if (legacy_gpu_database[i].device_id == device_id)
            return legacy_gpu_database[i].radeon_name;
    }
    return "Unknown Legacy GPU";
}

/**
 * amdgpu_legacy_gpu_codename() - Get internal codename
 *
 * Example: "Tahiti", "Bonaire"
 */
const char* amdgpu_legacy_gpu_codename(uint16_t device_id)
{
    for (size_t i = 0; legacy_gpu_database[i].device_id != 0; i++) {
        if (legacy_gpu_database[i].device_id == device_id)
            return legacy_gpu_database[i].codename;
    }
    return NULL;
}

/*
 * ============================================================================
 * FEATURE FLAG QUERIES
 * ============================================================================
 *
 * These functions allow code to determine what features are available
 * on a specific legacy GPU architecture.
 */

bool amdgpu_legacy_gpu_has_vce(uint16_t device_id)
{
    struct amd_legacy_gpu_info info;
    if (amdgpu_legacy_gpu_info_lookup(device_id, &info) != 0)
        return false;
    return (info.feature_flags & AMDGPU_LEGACY_FEATURE_VCE) != 0;
}

bool amdgpu_legacy_gpu_has_uvd(uint16_t device_id)
{
    struct amd_legacy_gpu_info info;
    if (amdgpu_legacy_gpu_info_lookup(device_id, &info) != 0)
        return false;
    return (info.feature_flags & AMDGPU_LEGACY_FEATURE_UVD) != 0;
}

bool amdgpu_legacy_gpu_has_dce(uint16_t device_id)
{
    struct amd_legacy_gpu_info info;
    if (amdgpu_legacy_gpu_info_lookup(device_id, &info) != 0)
        return false;
    return (info.feature_flags & AMDGPU_LEGACY_FEATURE_DCE) != 0;
}

bool amdgpu_legacy_gpu_has_sdma(uint16_t device_id)
{
    struct amd_legacy_gpu_info info;
    if (amdgpu_legacy_gpu_info_lookup(device_id, &info) != 0)
        return false;
    return (info.feature_flags & AMDGPU_LEGACY_FEATURE_SDMA) != 0;
}

bool amdgpu_legacy_gpu_has_doorbell(uint16_t device_id)
{
    struct amd_legacy_gpu_info info;
    if (amdgpu_legacy_gpu_info_lookup(device_id, &info) != 0)
        return false;
    return (info.feature_flags & AMDGPU_LEGACY_FEATURE_DOORBELL) != 0;
}

bool amdgpu_legacy_gpu_has_atc(uint16_t device_id)
{
    struct amd_legacy_gpu_info info;
    if (amdgpu_legacy_gpu_info_lookup(device_id, &info) != 0)
        return false;
    return (info.feature_flags & AMDGPU_LEGACY_FEATURE_ATC) != 0;
}

/*
 * ============================================================================
 * DIAGNOSTIC FUNCTIONS
 * ============================================================================
 */

/**
 * amdgpu_legacy_gpu_print_info() - Print detailed GPU information
 *
 * Useful for debugging and driver initialization messages.
 */
void amdgpu_legacy_gpu_print_info(uint16_t device_id)
{
    struct amd_legacy_gpu_info info;
    
    if (amdgpu_legacy_gpu_info_lookup(device_id, &info) != 0) {
        printf("[AMDGPU] Device 0x%04X: Not in legacy database\n", device_id);
        return;
    }

    printf("[AMDGPU Legacy GPU]\n");
    printf("  Device ID: 0x%04X\n", device_id);
    printf("  Codename: %s\n", info.codename);
    printf("  Marketing: %s\n", info.radeon_name);
    printf("  Generation: %s (GFX v%d)\n", info.generation, info.gfx_version);
    printf("  Driver: %s (default) / AMDGPU (if %s=1)\n",
           info.radeon_driver_supported ? "Radeon" : "None",
           info.module_param);
    printf("  Features:");
    if (info.feature_flags & AMDGPU_LEGACY_FEATURE_VCE) printf(" VCE");
    if (info.feature_flags & AMDGPU_LEGACY_FEATURE_UVD) printf(" UVD");
    if (info.feature_flags & AMDGPU_LEGACY_FEATURE_DCE) printf(" DCE");
    if (info.feature_flags & AMDGPU_LEGACY_FEATURE_SDMA) printf(" SDMA");
    if (info.feature_flags & AMDGPU_LEGACY_FEATURE_DOORBELL) printf(" Doorbell");
    if (info.feature_flags & AMDGPU_LEGACY_FEATURE_ATC) printf(" ATC");
    printf("\n");
}

/*
 * ============================================================================
 * MODULE INITIALIZATION / SYSCTL (for FreeBSD/Linux)
 * ============================================================================
 *
 * These sections would be expanded in the actual driver integration
 * to expose si_support and cik_support as module parameters.
 *
 * Linux:
 *   module_param(amdgpu_si_support, int, 0444);
 *   module_param(amdgpu_cik_support, int, 0444);
 *
 * FreeBSD:
 *   SYSCTL_INT(_hw_amdgpu, OID_AUTO, si_support, ...)
 *   SYSCTL_INT(_hw_amdgpu, OID_AUTO, cik_support, ...)
 */

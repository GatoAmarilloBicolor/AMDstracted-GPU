#include "amd_device.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Complete AMD GPU Device Database */
static const amd_gpu_device_info_t amd_device_database[] = {
    /* VLIW4/2 Legacy (2005-2012) */
    {
        .vendor_id = 0x1002, .device_id = 0x9806, .revision = 0,
        .generation = AMD_VLIW, .codename = "Wrestler",
        .marketing_name = "Radeon HD 7290", .max_compute_units = 1,
        .max_wave64_per_cu = 40, .max_vram_mb = 512,
        .capabilities = {
            .has_radv = false, .has_mesa = true, 
            .has_compute = false, .has_raytracing = false, .has_display = true
        },
        .preferred_backend = AMD_BACKEND_MESA
    },
    {
        .vendor_id = 0x1002, .device_id = 0x68c0, .revision = 0,
        .generation = AMD_VLIW, .codename = "Northern Islands",
        .marketing_name = "Radeon HD 6450", .max_compute_units = 2,
        .max_wave64_per_cu = 40, .max_vram_mb = 1024,
        .capabilities = {
            .has_radv = false, .has_mesa = true,
            .has_compute = false, .has_raytracing = false, .has_display = true
        },
        .preferred_backend = AMD_BACKEND_MESA
    },

    /* GCN1 - Southern Islands (2012-2013) */
    {
        .vendor_id = 0x1002, .device_id = 0x6798, .revision = 0,
        .generation = AMD_GCN1, .codename = "Tahiti",
        .marketing_name = "Radeon HD 7970", .max_compute_units = 32,
        .max_wave64_per_cu = 40, .max_vram_mb = 3072,
        .capabilities = {
            .has_radv = false, .has_mesa = true,
            .has_compute = true, .has_raytracing = false, .has_display = true
        },
        .preferred_backend = AMD_BACKEND_MESA
    },

    /* GCN2 - Sea Islands (2013-2014) */
    {
        .vendor_id = 0x1002, .device_id = 0x1638, .revision = 0,
        .generation = AMD_GCN2, .codename = "Hawaii",
        .marketing_name = "Radeon R9 290X", .max_compute_units = 44,
        .max_wave64_per_cu = 40, .max_vram_mb = 4096,
        .capabilities = {
            .has_radv = false, .has_mesa = true,
            .has_compute = true, .has_raytracing = false, .has_display = true
        },
        .preferred_backend = AMD_BACKEND_MESA
    },

    /* GCN3 - Fiji/Polaris (2015) */
    {
        .vendor_id = 0x1002, .device_id = 0x7300, .revision = 0,
        .generation = AMD_GCN3, .codename = "Fiji",
        .marketing_name = "Radeon R9 Fury", .max_compute_units = 64,
        .max_wave64_per_cu = 40, .max_vram_mb = 4096,
        .capabilities = {
            .has_radv = false, .has_mesa = true,
            .has_compute = true, .has_raytracing = false, .has_display = true
        },
        .preferred_backend = AMD_BACKEND_MESA
    },
    {
        .vendor_id = 0x1002, .device_id = 0x67ff, .revision = 0,
        .generation = AMD_GCN3, .codename = "Polaris",
        .marketing_name = "Radeon RX 480", .max_compute_units = 36,
        .max_wave64_per_cu = 40, .max_vram_mb = 8192,
        .capabilities = {
            .has_radv = false, .has_mesa = true,
            .has_compute = true, .has_raytracing = false, .has_display = true
        },
        .preferred_backend = AMD_BACKEND_MESA
    },

    /* GCN4 - Vega (2016) */
    {
        .vendor_id = 0x1002, .device_id = 0x687f, .revision = 0,
        .generation = AMD_GCN4, .codename = "Vega10",
        .marketing_name = "Radeon RX Vega 64", .max_compute_units = 64,
        .max_wave64_per_cu = 40, .max_vram_mb = 8192,
        .capabilities = {
            .has_radv = true, .has_mesa = true,
            .has_compute = true, .has_raytracing = false, .has_display = true
        },
        .preferred_backend = AMD_BACKEND_RADV
    },

    /* GCN5 - RDNA Gen 1 (2019) */
    {
        .vendor_id = 0x1002, .device_id = 0x7340, .revision = 0,
        .generation = AMD_GCN5, .codename = "Navi10",
        .marketing_name = "Radeon RX 5700 XT", .max_compute_units = 40,
        .max_wave64_per_cu = 40, .max_vram_mb = 8192,
        .capabilities = {
            .has_radv = true, .has_mesa = true,
            .has_compute = true, .has_raytracing = false, .has_display = true
        },
        .preferred_backend = AMD_BACKEND_RADV
    },

    /* RDNA2 (2020-2021) */
    {
        .vendor_id = 0x1002, .device_id = 0x164c, .revision = 0,
        .generation = AMD_RDNA2, .codename = "Lucienne",
        .marketing_name = "Radeon Mobile", .max_compute_units = 8,
        .max_wave64_per_cu = 40, .max_vram_mb = 2048,
        .capabilities = {
            .has_radv = true, .has_mesa = true,
            .has_compute = true, .has_raytracing = true, .has_display = true
        },
        .preferred_backend = AMD_BACKEND_RADV
    },
    {
        .vendor_id = 0x1002, .device_id = 0x73bf, .revision = 0,
        .generation = AMD_RDNA2, .codename = "Sienna Cichlid",
        .marketing_name = "Radeon RX 6800 XT", .max_compute_units = 72,
        .max_wave64_per_cu = 40, .max_vram_mb = 16384,
        .capabilities = {
            .has_radv = true, .has_mesa = true,
            .has_compute = true, .has_raytracing = true, .has_display = true
        },
        .preferred_backend = AMD_BACKEND_RADV
    },

    /* RDNA3 (2022+) */
    {
        .vendor_id = 0x1002, .device_id = 0x741f, .revision = 0,
        .generation = AMD_RDNA3, .codename = "Navi31",
        .marketing_name = "Radeon RX 7900 XT", .max_compute_units = 96,
        .max_wave64_per_cu = 40, .max_vram_mb = 24576,
        .capabilities = {
            .has_radv = true, .has_mesa = true,
            .has_compute = true, .has_raytracing = true, .has_display = true
        },
        .preferred_backend = AMD_BACKEND_RADV
    },
};

static const size_t amd_device_count = 
    sizeof(amd_device_database) / sizeof(amd_device_database[0]);

/* Device Lookup */
amd_gpu_device_info_t* amd_device_lookup(uint16_t device_id)
{
    for (size_t i = 0; i < amd_device_count; i++) {
        if (amd_device_database[i].device_id == device_id) {
            return (amd_gpu_device_info_t*)&amd_device_database[i];
        }
    }
    return NULL;
}

/* Backend Selection */
amd_backend_type_t amd_select_backend(amd_gpu_device_info_t *gpu)
{
    if (!gpu) return AMD_BACKEND_SOFTWARE;
    
    /* Modern GPUs prefer RADV */
    if (gpu->generation >= AMD_RDNA2 && gpu->capabilities.has_radv) {
        return AMD_BACKEND_RADV;
    }
    
    /* Fall back to Mesa */
    if (gpu->capabilities.has_mesa) {
        return AMD_BACKEND_MESA;
    }
    
    /* Last resort */
    return AMD_BACKEND_SOFTWARE;
}

/* Debug: Print device info */
void amd_device_print_info(amd_gpu_device_info_t *gpu)
{
    if (!gpu) {
        printf("Unknown device\n");
        return;
    }
    
    printf("AMD GPU Info:\n");
    printf("  Device ID: [%04x:%04x]\n", gpu->vendor_id, gpu->device_id);
    printf("  Codename: %s\n", gpu->codename);
    printf("  Marketing: %s\n", gpu->marketing_name);
    printf("  Generation: %d\n", gpu->generation);
    printf("  Compute Units: %u\n", gpu->max_compute_units);
    printf("  Max VRAM: %u MB\n", gpu->max_vram_mb);
    printf("  RADV Support: %s\n", gpu->capabilities.has_radv ? "yes" : "no");
    printf("  Mesa Support: %s\n", gpu->capabilities.has_mesa ? "yes" : "no");
    printf("  Compute: %s\n", gpu->capabilities.has_compute ? "yes" : "no");
    printf("  Ray Tracing: %s\n", gpu->capabilities.has_raytracing ? "yes" : "no");
    printf("  Display: %s\n", gpu->capabilities.has_display ? "yes" : "no");
    printf("  Preferred Backend: %d\n", gpu->preferred_backend);
}

/*
 * AMDGPU Radeon Legacy Support Integration Example
 *
 * This file demonstrates how to integrate the legacy GPU support layer
 * into the AMDGPU driver at key decision points.
 *
 * Use case: Determining whether AMDGPU should claim a GPU or defer to Radeon
 */

#include <stdio.h>
#include <stdint.h>

/* Include the legacy support abstraction */
#include "../src/amd/radeon_legacy_support.h"

/*
 * ============================================================================
 * EXAMPLE 1: PCI Device Probe (Simplified)
 * ============================================================================
 *
 * This simulates the logic in amdgpu_pci_probe() where the driver
 * decides whether to claim or defer a device.
 *
 * Reference: amdgpu_drv.c:2260-2300
 */

int simulate_pci_probe(uint16_t pci_device_id, uint32_t chip_id)
{
    const char *generation;
    struct amd_legacy_gpu_info info;
    
    printf("[AMDGPU PCI Probe Simulation]\n");
    printf("  Device ID: 0x%04X\n", pci_device_id);

    /* Step 1: Check if this is a legacy GPU */
    if (!amdgpu_is_legacy_gpu(pci_device_id)) {
        printf("  Status: Modern GPU - AMDGPU claims it\n");
        return 0;  /* AMDGPU_DRV_OK */
    }

    /* Step 2: Get generation information */
    generation = amdgpu_legacy_gpu_generation(pci_device_id);
    if (!generation) {
        printf("  Status: Unknown GPU\n");
        return 0;  /* Claim it anyway */
    }

    /* Step 3: Check if driver should defer to Radeon */
    if (amdgpu_should_defer_to_radeon(chip_id, generation)) {
        printf("  Generation: %s\n", generation);
        printf("  Status: DEFERRING to Radeon driver\n");
        
        /* Log which message to print */
        if (strcmp(generation, "SI") == 0) {
            printf("  Message: \"%s\"\n", AMDGPU_LEGACY_SI_DEFER_MSG);
        } else if (strcmp(generation, "CIK") == 0) {
            printf("  Message: \"%s\"\n", AMDGPU_LEGACY_CIK_DEFER_MSG);
        }
        
        return -19;  /* -ENODEV: Let kernel try next driver */
    }

    /* Step 4: AMDGPU will claim this device */
    if (amdgpu_legacy_gpu_info_lookup(pci_device_id, &info) == 0) {
        printf("  Generation: %s\n", generation);
        printf("  Name: %s\n", info.radeon_name);
        printf("  Status: AMDGPU claiming (module param override)\n");
        
        if (strcmp(generation, "SI") == 0) {
            printf("  Message: \"%s\"\n", AMDGPU_LEGACY_SI_ENABLE_MSG);
        } else if (strcmp(generation, "CIK") == 0) {
            printf("  Message: \"%s\"\n", AMDGPU_LEGACY_CIK_ENABLE_MSG);
        }
    }
    
    return 0;  /* AMDGPU_DRV_OK */
}

/*
 * ============================================================================
 * EXAMPLE 2: Feature Detection
 * ============================================================================
 *
 * Before initializing specific hardware subsystems, check if the GPU
 * actually supports them.
 */

void initialize_gpu_features(uint16_t device_id)
{
    printf("[Feature Initialization]\n");
    printf("  Device ID: 0x%04X\n", device_id);

    struct amd_legacy_gpu_info info;
    if (amdgpu_legacy_gpu_info_lookup(device_id, &info) != 0) {
        printf("  Status: Not a legacy GPU\n");
        return;
    }

    printf("  Name: %s\n", info.radeon_name);
    printf("  Features:\n");

    if (amdgpu_legacy_gpu_has_vce(device_id)) {
        printf("    ✓ VCE (Video Codec Engine) - Enabling\n");
        /* init_vce_engine(); */
    } else {
        printf("    ✗ VCE not available\n");
    }

    if (amdgpu_legacy_gpu_has_uvd(device_id)) {
        printf("    ✓ UVD (Unified Video Decoder) - Enabling\n");
        /* init_uvd_decoder(); */
    } else {
        printf("    ✗ UVD not available\n");
    }

    if (amdgpu_legacy_gpu_has_dce(device_id)) {
        printf("    ✓ DCE (Display Core Engine) - Enabling\n");
        /* init_dce(); */
    } else {
        printf("    ✗ DCE not available\n");
    }

    if (amdgpu_legacy_gpu_has_sdma(device_id)) {
        printf("    ✓ SDMA (System DMA) - Enabling\n");
        /* init_sdma(); */
    } else {
        printf("    ✗ SDMA not available\n");
    }

    if (amdgpu_legacy_gpu_has_doorbell(device_id)) {
        printf("    ✓ Doorbell (Command Submission) - Enabling\n");
        /* init_doorbells(); */
    } else {
        printf("    ✗ Doorbell not available\n");
    }

    if (amdgpu_legacy_gpu_has_atc(device_id)) {
        printf("    ✓ ATC (Address Translation Cache) - Enabling\n");
        /* init_atc(); */
    } else {
        printf("    ✗ ATC not available\n");
    }
}

/*
 * ============================================================================
 * EXAMPLE 3: Hardware-Specific Initialization
 * ============================================================================
 *
 * Different legacy GPU families may need specific register setup or
 * IP block initialization.
 */

void initialize_legacy_gpu_ip_blocks(uint16_t device_id)
{
    printf("[IP Block Initialization]\n");
    printf("  Device ID: 0x%04X\n", device_id);

    struct amd_legacy_gpu_info info;
    if (amdgpu_legacy_gpu_info_lookup(device_id, &info) != 0) {
        printf("  Status: Not a legacy GPU - using default init\n");
        return;
    }

    printf("  Name: %s (%s)\n", info.codename, info.radeon_name);
    printf("  GFX Version: %d\n", info.gfx_version);

    /* Initialize based on architecture version */
    switch (info.gfx_version) {
        case 6:
            printf("  Architecture: GCN v1 (Southern Islands)\n");
            printf("  Initializing SI-specific IP blocks:\n");
            printf("    - GFX Engine v6\n");
            printf("    - DCE v6 (Display)\n");
            printf("    - SDMA (legacy variant)\n");
            /* si_init_ip_blocks(adev); */
            break;

        case 7:
            printf("  Architecture: GCN v2 (Sea Islands)\n");
            printf("  Initializing CIK-specific IP blocks:\n");
            printf("    - GFX Engine v7\n");
            printf("    - DCE v8 (Display)\n");
            printf("    - SDMA (improved)\n");
            printf("    - Doorbells\n");
            /* cik_init_ip_blocks(adev); */
            break;

        default:
            printf("  Architecture: Unknown version %d\n", info.gfx_version);
            break;
    }
}

/*
 * ============================================================================
 * EXAMPLE 4: Device Database Enumeration
 * ============================================================================
 *
 * Iterate through all known legacy GPUs to print capabilities.
 * Useful for documentation, debugging, and verification.
 */

void print_legacy_gpu_database(void)
{
    printf("[Legacy GPU Database]\n\n");

    size_t count = 0;
    const struct amd_legacy_gpu_info *db = amdgpu_legacy_gpu_list(&count);
    
    if (!db) {
        printf("  Error: Could not retrieve legacy GPU database\n");
        return;
    }

    printf("  Total Legacy GPUs: %zu\n\n", count);

    printf("  %-12s | %-15s | Device  | Gen  | Features\n", 
           "Codename", "Marketing");
    printf("  %-12s | %-15s | ------- | ---- | -------\n",
           "--------", "---------");

    for (size_t i = 0; i < count; i++) {
        const struct amd_legacy_gpu_info *gpu = &db[i];
        
        /* Build feature string */
        char features[128] = "";
        if (gpu->feature_flags & AMDGPU_LEGACY_FEATURE_VCE) strcat(features, "VCE ");
        if (gpu->feature_flags & AMDGPU_LEGACY_FEATURE_UVD) strcat(features, "UVD ");
        if (gpu->feature_flags & AMDGPU_LEGACY_FEATURE_DCE) strcat(features, "DCE ");
        if (gpu->feature_flags & AMDGPU_LEGACY_FEATURE_SDMA) strcat(features, "SDMA ");
        if (gpu->feature_flags & AMDGPU_LEGACY_FEATURE_DOORBELL) strcat(features, "DB ");
        if (gpu->feature_flags & AMDGPU_LEGACY_FEATURE_ATC) strcat(features, "ATC");

        printf("  %-12s | %-15s | 0x%04X  | GFX%d | %s\n",
               gpu->codename, gpu->radeon_name, gpu->device_id,
               gpu->gfx_version, features);
    }

    printf("\n  Legend:\n");
    printf("    VCE = Video Codec Engine\n");
    printf("    UVD = Unified Video Decoder\n");
    printf("    DCE = Display Core Engine\n");
    printf("    SDMA = System DMA\n");
    printf("    DB  = Doorbell (command submission)\n");
    printf("    ATC = Address Translation Cache\n");
}

/*
 * ============================================================================
 * EXAMPLE 5: Module Parameter Override Detection
 * ============================================================================
 *
 * Determine the current state of si_support and cik_support parameters
 * for diagnostic purposes.
 */

void print_module_parameter_status(void)
{
    printf("[Module Parameter Status]\n");
    printf("  amdgpu_si_support = %d (default: 0)\n", amdgpu_si_support);
    printf("  amdgpu_cik_support = %d (default: 0)\n", amdgpu_cik_support);

    printf("\n  Current Behavior:\n");
    
    if (amdgpu_si_support == 0) {
        printf("    SI (Southern Islands):  DEFERRED to Radeon driver\n");
    } else {
        printf("    SI (Southern Islands):  AMDGPU will claim devices\n");
    }

    if (amdgpu_cik_support == 0) {
        printf("    CIK (Sea Islands):      DEFERRED to Radeon driver\n");
    } else {
        printf("    CIK (Sea Islands):      AMDGPU will claim devices\n");
    }

    printf("\n  To Override:\n");
    printf("    modprobe amdgpu si_support=1 cik_support=1\n");
}

/*
 * ============================================================================
 * MAIN: Integration Demonstration
 * ============================================================================
 */

int main(void)
{
    printf("====================================================\n");
    printf("AMDGPU Radeon Legacy Support Integration Examples\n");
    printf("====================================================\n\n");

    /* Example 1: PCI Probe Decision Making */
    printf("\n--- EXAMPLE 1: PCI Device Probe ---\n\n");
    
    printf("Test Case 1a: Tahiti (0x6798) - SI Legacy GPU\n");
    simulate_pci_probe(0x6798, CHIP_TAHITI);
    
    printf("\nTest Case 1b: Hawaii (0x67B0) - CIK Legacy GPU\n");
    simulate_pci_probe(0x67B0, CHIP_HAWAII);
    
    printf("\nTest Case 1c: Polaris 10 (0x67FF) - Modern GPU\n");
    simulate_pci_probe(0x67FF, 0xFFFFFFFF);  /* Unknown chip */

    /* Example 2: Feature Detection */
    printf("\n--- EXAMPLE 2: Feature Detection ---\n\n");
    initialize_gpu_features(0x6798);  /* Tahiti */

    printf("\n");
    initialize_gpu_features(0x67B0);  /* Hawaii */

    /* Example 3: Hardware-Specific Init */
    printf("\n--- EXAMPLE 3: Hardware-Specific Initialization ---\n\n");
    initialize_legacy_gpu_ip_blocks(0x6798);  /* Tahiti */
    
    printf("\n");
    initialize_legacy_gpu_ip_blocks(0x67B0);  /* Hawaii */

    /* Example 4: Database Enumeration */
    printf("\n--- EXAMPLE 4: Legacy GPU Database ---\n\n");
    print_legacy_gpu_database();

    /* Example 5: Module Parameters */
    printf("\n--- EXAMPLE 5: Module Parameters ---\n\n");
    print_module_parameter_status();

    printf("\n====================================================\n");
    printf("Integration Examples Complete\n");
    printf("====================================================\n");

    return 0;
}

/*
 * Compilation (in project root):
 *   gcc -I. -o radeon_legacy_example \
 *       examples/radeon_legacy_example.c \
 *       src/amd/radeon_legacy_support.c
 *
 * Execution:
 *   ./radeon_legacy_example
 */

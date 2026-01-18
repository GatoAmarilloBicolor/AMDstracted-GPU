#include "hal.h"
#include "../os/os_primitives.h"
#include "../amdgpu_pci_ids.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Developed by: Haiku Imposible Team (HIT)
 * This is the HAL "Factory". It handles all the specialist workers (IP Blocks).
 */

// Adding a new specialist worker to our GPU team
int amdgpu_device_ip_block_add(
    struct OBJGPU *adev, const struct amd_ip_block_version *ip_block_version) {
  if (adev->num_ip_blocks >= AMDGPU_MAX_IP_BLOCKS)
    return -1; // Too many workers!
  adev->ip_blocks[adev->num_ip_blocks].version = ip_block_version;
  adev->ip_blocks[adev->num_ip_blocks].status = false; // Not started yet
  adev->num_ip_blocks++;
  return 0;
}

/* ============================================================================
 * Import real IP Block implementations from separate files
 * ============================================================================ */

// Forward declarations (implemented in gmc_v10.c and gfx_v10.c)
extern const struct amd_ip_block_version gmc_v10_ip_block;
extern const struct amd_ip_block_version gfx_v10_ip_block;

/* --- Navi10 Specialist Skills (The Actual Code) --- */

// 1. The Manager (Common Block)
static int navi10_common_early_init(struct OBJGPU *adev) {
  os_prim_log("HAL: [Manager] Checking if the GPU is awake...\n");
  return 0;
}

static int navi10_common_sw_init(struct OBJGPU *adev) {
  os_prim_log("HAL: [Manager] Setting up the software workspace.\n");
  return 0;
}

static int navi10_common_hw_init(struct OBJGPU *adev) {
  os_prim_log("HAL: [Manager] Starting the hardware engines!\n");
  return 0;
}

static int navi10_common_late_init(struct OBJGPU *adev) {
  os_prim_log("HAL: [Manager] Doing the final polish.\n");
  return 0;
}

static int navi10_common_hw_fini(struct OBJGPU *adev) {
  os_prim_log("HAL: [Manager] Powering down engines... See ya!\n");
  return 0;
}

static const struct amd_ip_funcs navi10_common_ip_funcs = {
    .name = "navi10_common",
    .early_init = navi10_common_early_init,
    .sw_init = navi10_common_sw_init,
    .hw_init = navi10_common_hw_init,
    .late_init = navi10_common_late_init,
    .hw_fini = navi10_common_hw_fini,
};

static const struct amd_ip_block_version navi10_common_ip_block = {
    .type = AMD_IP_BLOCK_TYPE_COMMON,
    .major = 1,
    .minor = 0,
    .rev = 0,
    .funcs = &navi10_common_ip_funcs,
};

/* --- Wrestler (Family 14h / APU) Specialist Skills --- */

static int wrestler_common_early_init(struct OBJGPU *adev) {
  os_prim_log(
      "HAL: [Wrestler Manager] Hello, little APU! Checking systems...\n");
  return 0;
}

static int wrestler_common_hw_fini(struct OBJGPU *adev) {
  os_prim_log("HAL: [Wrestler Manager] APU system shut down successfully.\n");
  return 0;
}

static const struct amd_ip_funcs wrestler_common_ip_funcs = {
    .name = "wrestler_common",
    .early_init = wrestler_common_early_init,
    .hw_init = navi10_common_hw_init, // Reusing high-level warm-up
    .hw_fini = wrestler_common_hw_fini,
};

static const struct amd_ip_block_version wrestler_common_ip_block = {
    .type = AMD_IP_BLOCK_TYPE_COMMON,
    .major = 1,
    .minor = 0,
    .rev = 0,
    .funcs = &wrestler_common_ip_funcs,
};

/* --- Legacy Radeon (R600/Evergreen/NI) SKills --- */

static int legacy_radeon_common_hw_init(struct OBJGPU *adev) {
  os_prim_log("HAL: [Radeon Manager] Waking up legacy hardware "
              "(Evergreen/NI/R600)...\n");
  os_prim_log(
      "HAL: [Radeon Manager] Loading microcode bits for the old guard.\n");
  return 0;
}

static const struct amd_ip_funcs legacy_radeon_common_ip_funcs = {
    .name = "legacy_radeon_common",
    .early_init = wrestler_common_early_init,
    .hw_init = legacy_radeon_common_hw_init,
    .hw_fini = wrestler_common_hw_fini,
};

static const struct amd_ip_block_version legacy_radeon_common_ip_block = {
    .type = AMD_IP_BLOCK_TYPE_COMMON,
    .major = 1,
    .minor = 0,
    .rev = 0,
    .funcs = &legacy_radeon_common_ip_funcs,
};

/* --- The Main HAL Commands --- */

// Turning on the whole GPU city!
int amdgpu_device_init_hal(struct OBJGPU *adev) {
  if (!adev)
    return -1;

  adev->num_ip_blocks = 0;
  adev->res_root = rs_resource_create(0, NULL);
  /* Synchronization handled by os_prim_lock/unlock when needed */

  os_prim_log("HAL: Starting the GPU City (HIT Edition) - Let's gooooo!\n");

  // Discovery: Find who this GPU is!
  uint16_t vendor, device;
  if (adev->pci_handle) {
    os_prim_pci_get_ids(adev->pci_handle, &vendor, &device);
  } else {
    // In some cases (simulation), we might not have a handle yet
    vendor = 0x1002;
    device = 0x7310; // Generic Navi10
  }
  adev->device_id = device;

  const struct amd_pci_info *pci_info = NULL;
  for (int i = 0; amd_pci_table[i].device_id != 0 || i == 0; i++) {
    if (amd_pci_table[i].device_id == device ||
        amd_pci_table[i].device_id == 0) {
      pci_info = &amd_pci_table[i];
      adev->asic_type = pci_info->asic_type;
      break;
    }
  }

  os_prim_log("HAL: Identified GPU: %s\n",
              pci_info ? pci_info->name : "Unknown");

  // Registering Specialists based on ASIC
  if (adev->asic_type == AMD_ASIC_NAVI10) {
    os_prim_log("HAL: Loading NAVI10 specialists (GMC v10, GFX v10)...\n");
    amdgpu_device_ip_block_add(adev, &navi10_common_ip_block);
    amdgpu_device_ip_block_add(adev, &gmc_v10_ip_block);  // Real GMC v10
    amdgpu_device_ip_block_add(adev, &gfx_v10_ip_block);  // Real GFX v10
  } else if (adev->asic_type == AMD_ASIC_WRESTLER) {
    os_prim_log("HAL: Loading Wrestler APU specialists...\n");
    amdgpu_device_ip_block_add(adev, &wrestler_common_ip_block);
    amdgpu_device_ip_block_add(adev, &gmc_v10_ip_block);  // Using v10 for APU too
  } else {
    // Legacy Radeon Path
    os_prim_log("HAL: Loading legacy Radeon specialists...\n");
    amdgpu_device_ip_block_add(adev, &legacy_radeon_common_ip_block);
    amdgpu_device_ip_block_add(adev, &gmc_v10_ip_block);  // Even legacy needs GMC
  }
  // --- Hardware Link: Mapping the MMIO Registers ---
  os_prim_log("HAL: Connecting to hardware registers (MMIO Mapping)...\n");
  // We use our PCI handle (simulated or real) to map BAR 0 or 2
  adev->mmio_base = os_prim_pci_map_resource((void *)0x9806, 0);

  if (!adev->mmio_base) {
    os_prim_log("HAL: Error! Failed to map GPU registers. Aborting.\n");
    return -1;
  }

  // Double check our connection with a poke!
  os_prim_log("HAL: Poking hardware... Hello?\n");

  // SAFETY CHECK: Only write if the offset is within a known safe range
  // (example)
  uintptr_t poke_addr = (uintptr_t)adev->mmio_base + 0x100;
  if (poke_addr >= (uintptr_t)adev->mmio_base &&
      poke_addr < (uintptr_t)adev->mmio_base + 0x100000) {
    os_prim_write32(poke_addr, 0x1); // Send identifying signal
    os_prim_read32(poke_addr);
  } else {
    os_prim_log("HAL: [SAFETY] Blocked illegal hardware poke at out-of-bounds "
                "address!\n");
  }

  // The 4-Step Start Sequence (Like a professional athlete's warm-up)

  // Step 1: Tell everyone to wake up
  for (int i = 0; i < adev->num_ip_blocks; i++) {
    if (adev->ip_blocks[i].version->funcs->early_init)
      adev->ip_blocks[i].version->funcs->early_init(adev);
  }

  // Step 2: Set up the software workspace
  for (int i = 0; i < adev->num_ip_blocks; i++) {
    if (adev->ip_blocks[i].version->funcs->sw_init)
      adev->ip_blocks[i].version->funcs->sw_init(adev);
  }

  // Step 3: Flip the switches! (Hardware start)
  for (int i = 0; i < adev->num_ip_blocks; i++) {
    if (adev->ip_blocks[i].version->funcs->hw_init) {
      adev->ip_blocks[i].version->funcs->hw_init(adev);
      adev->ip_blocks[i].status = true; // High-five! They are working.
    }
  }

  // Step 4: Final checks before we start for real
  for (int i = 0; i < adev->num_ip_blocks; i++) {
    if (adev->ip_blocks[i].version->funcs->late_init)
      adev->ip_blocks[i].version->funcs->late_init(adev);
  }

  // Initialize GPU Info (Phase 2.2 - for framebuffer management)
  amdgpu_gpu_get_info_hal(adev, &adev->gpu_info);
  os_prim_log("HAL: GPU info cached - VRAM: %uMB @ 0x%llx, Clock: %uMHz\n",
              adev->gpu_info.vram_size_mb, adev->gpu_info.vram_base,
              adev->gpu_info.gpu_clock_mhz);

  // Belter Strategy: Initialize State
  adev->state = AMD_GPU_STATE_RUNNING;
  memset(&adev->shadow, 0, sizeof(adev->shadow));
  /* Heartbeat thread can be spawned via os_prim_spawn_thread if needed */
  /* For now, running synchronously */

  return 0;
}

// Shutting down the city for the night
void amdgpu_device_fini_hal(struct OBJGPU *adev) {
  for (int i = adev->num_ip_blocks - 1; i >= 0; i--) {
    if (adev->ip_blocks[i].status &&
        adev->ip_blocks[i].version->funcs->hw_fini) {
      adev->ip_blocks[i].version->funcs->hw_fini(adev);
    }
  }
  rs_resource_destroy(adev->res_root);
  /* Synchronization cleanup handled by os_prim_cleanup if needed */
}

// Just asking the GPU "Who are you?"
int amdgpu_gpu_get_info_hal(struct OBJGPU *adev, amdgpu_gpu_info_t *info) {
  os_prim_log("HAL: [Manager] Giving out the GPU ID card.\n");

  const struct amd_pci_info *pci_info = NULL;
  for (int i = 0; amd_pci_table[i].device_id != 0; i++) {
    if (amd_pci_table[i].device_id == adev->device_id) {
      pci_info = &amd_pci_table[i];
      break;
    }
  }

  if (pci_info) {
    info->vram_size_mb = pci_info->vram_mb_default;
    info->gpu_clock_mhz = pci_info->clock_mhz;
    strncpy(info->gpu_name, pci_info->name, 31);
    // In a real kernel driver, this would be adev->resource[BAR0].start
    // For our userland abstraction, we provide a consistent base.
    info->vram_base = 0xE0000000;
  } else {
    // Ultimate fallback
    info->vram_size_mb = 1024;
    info->gpu_clock_mhz = 1000;
    strncpy(info->gpu_name, "Generic AMD GPU", 31);
    info->vram_base = 0xE0000000;
  }
  return 0;
}

// Asking the Librarian for some space (Memory allocation)
int amdgpu_buffer_alloc_hal(struct OBJGPU *adev, size_t size,
                            struct amdgpu_buffer *buf) {
  os_prim_log("HAL: [Librarian] Finding a spot for your data...\n");
  buf->cpu_addr = os_prim_alloc(size);
  buf->gpu_addr = (uint64_t)buf->cpu_addr; // Magic transformation!
  buf->size = size;
  return buf->cpu_addr ? 0 : -1;
}

void amdgpu_buffer_free_hal(struct OBJGPU *adev, struct amdgpu_buffer *buf) {
  os_prim_log("HAL: [Librarian] Giving the space back to the library.\n");
  if (buf->cpu_addr)
    os_prim_free(buf->cpu_addr);
}

// Sending a list of jobs to the Artist (Graphics/Compute)
int amdgpu_command_submit_hal(struct OBJGPU *adev,
                              struct amdgpu_command_buffer *cb) {
  os_prim_log("HAL: [Artist] Got your draw calls! Processing now...\n");
  // Belter Strategy: Might want to shadow command submissions too!
  return 0;
}

// Setting the display mode (CRTC timing + scanout)
int amdgpu_set_display_mode_hal(struct OBJGPU *adev, const display_mode *mode) {
  os_prim_log("HAL: [Display Manager] Setting mode %ux%u\n", 
              mode->virtual_width, mode->virtual_height);
  
  if (!adev || !mode) {
    os_prim_log("HAL: [Display Manager] Invalid GPU or mode pointer!\n");
    return -1;
  }

  // Find the GFX block to program CRTC
  int gfx_block_idx = -1;
  for (int i = 0; i < adev->num_ip_blocks; i++) {
    if (adev->ip_blocks[i].version &&
        adev->ip_blocks[i].version->type == AMD_IP_BLOCK_TYPE_GFX) {
      gfx_block_idx = i;
      break;
    }
  }

  if (gfx_block_idx < 0) {
    os_prim_log("HAL: [Display Manager] GFX block not found!\n");
    return -1;
  }

  // Step 1: Program CRTC timing via GFX block
  int ret = gfx_v10_set_crtc_timing(adev, mode);
  
  if (ret != 0) {
    os_prim_log("HAL: [Display Manager] Failed to set CRTC timing (error %d)\n", ret);
    return ret;
  }
  
  os_prim_log("HAL: [Display Manager] CRTC timing set successfully\n");

  // Step 2: Program scanout address via GMC (Phase 2.2)
  // Use VRAM base from adev or from pre-allocated framebuffer
  // For now, use simple scanout address from GPU memory
  uint64_t scanout_addr = adev->gpu_info.vram_base;
  
  ret = gmc_v10_set_scanout_address(adev, scanout_addr);
  if (ret != 0) {
    os_prim_log("HAL: [Display Manager] Failed to set scanout address (error %d)\n", ret);
    return ret;
  }
  
  os_prim_log("HAL: [Display Manager] Scanout address set to 0x%llx\n", scanout_addr);
  os_prim_log("HAL: [Display Manager] Display mode set successfully!\n");
  
  return 0;
}

/* --- Belter "Self-Healing" Implementation --- */

// 1. Shadow Write: Mirror writes to RAM
void amdgpu_hal_shadow_write(struct OBJGPU *adev, uint32_t offset,
                             uint32_t value) {
  if (offset < 1024) {
    adev->shadow.regs[offset] = value;
    adev->shadow.valid[offset] = true;
  }
  // Write to real hardware
  uintptr_t poke_addr = (uintptr_t)adev->mmio_base + offset * 4;
  os_prim_write32(poke_addr, value);
}

// 2. Transparent Reset: The Lazarus Protocol
int amdgpu_hal_reset(struct OBJGPU *adev) {
  os_prim_log(
      "HAL: [Belter] CRITICAL! GPU hang detected. Initiating reset...\n");

  adev->state = AMD_GPU_STATE_RESETTING;

  // A. Stop the Engines
  amdgpu_device_fini_hal(adev);

  // B. Restart the Hardware
  os_prim_log("HAL: [Belter] Kickstarting the ASIC...\n");
  amdgpu_device_init_hal(adev);

  // C. Replay the Shadow State
  os_prim_log("HAL: [Belter] Replaying Shadow State to restore context...\n");
  for (int i = 0; i < 1024; i++) {
    if (adev->shadow.valid[i]) {
      uintptr_t poke_addr = (uintptr_t)adev->mmio_base + i * 4;
      os_prim_write32(poke_addr, adev->shadow.regs[i]);
    }
  }

  adev->state = AMD_GPU_STATE_RUNNING;
  os_prim_log("HAL: [Belter] GPU resurrection complete. We are back online.\n");
  return 0;
}

// 3. Heartbeat Monitor: Staying Alive
void *amdgpu_hal_heartbeat(void *arg) {
  struct OBJGPU *adev = (struct OBJGPU *)arg;
  while (1) {
    os_prim_delay_us(1000000); // Check every second

    // In a real driver, we would check fence values.
    // Here, we simulate a check.
    if (adev->state == AMD_GPU_STATE_HUNG) {
      amdgpu_hal_reset(adev);
    }
  }
  return NULL;
}
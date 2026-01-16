#include "hal.h"
#include "../../kernel-amd/os-primitives/os_primitives.h"
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

// 2. The Artist (GFX Block)
static int navi10_gfx_hw_init(struct OBJGPU *adev) {
  os_prim_log("HAL: [Artist] Ready to draw some cool 3D stuff!\n");
  return 0;
}

static int navi10_gfx_hw_fini(struct OBJGPU *adev) {
  os_prim_log("HAL: [Artist] Brushes cleaned and put away.\n");
  return 0;
}

static const struct amd_ip_funcs navi10_gfx_ip_funcs = {
    .name = "navi10_gfx",
    .hw_init = navi10_gfx_hw_init,
    .hw_fini = navi10_gfx_hw_fini,
};

static const struct amd_ip_block_version navi10_gfx_ip_block = {
    .type = AMD_IP_BLOCK_TYPE_GFX,
    .major = 10,
    .minor = 1,
    .rev = 0,
    .funcs = &navi10_gfx_ip_funcs,
};

// 3. The Librarian (GMC Block)
static int navi10_gmc_hw_init(struct OBJGPU *adev) {
  os_prim_log("HAL: [Librarian] Organizing the VRAM library.\n");
  return 0;
}

static int navi10_gmc_hw_fini(struct OBJGPU *adev) {
  os_prim_log("HAL: [Librarian] Library is closed for the night.\n");
  return 0;
}

static const struct amd_ip_funcs navi10_gmc_ip_funcs = {
    .name = "navi10_gmc",
    .hw_init = navi10_gmc_hw_init,
    .hw_fini = navi10_gmc_hw_fini,
};

static const struct amd_ip_block_version navi10_gmc_ip_block = {
    .type = AMD_IP_BLOCK_TYPE_GMC,
    .major = 10,
    .minor = 0,
    .rev = 0,
    .funcs = &navi10_gmc_ip_funcs,
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

/* --- The Main HAL Commands --- */

// Turning on the whole GPU city!
int amdgpu_device_init_hal(struct OBJGPU *adev) {
  os_prim_log("HAL: Starting the GPU City (HIT Edition) - Let's gooooo!\n");

  pthread_mutex_init(&adev->gpu_lock, NULL);
  adev->res_root = rs_resource_create(0, NULL);

  // Discovering which GPU chip we have
  if (adev->asic_type == AMD_ASIC_NAVI10) {
    amdgpu_device_ip_block_add(adev, &navi10_common_ip_block);
    amdgpu_device_ip_block_add(adev, &navi10_gmc_ip_block);
    amdgpu_device_ip_block_add(adev, &navi10_gfx_ip_block);
  } else if (adev->asic_type == AMD_ASIC_WRESTLER) {
    os_prim_log("HAL: Recognized your Radeon HD 7290 APU! 🌀\n");
    amdgpu_device_ip_block_add(adev, &wrestler_common_ip_block);
    amdgpu_device_ip_block_add(adev,
                               &navi10_gmc_ip_block); // APU uses GTT memory
  } else {
    os_prim_log(
        "HAL: Unknown chip type? Just using Navi10 defaults for now.\n");
    amdgpu_device_ip_block_add(adev, &navi10_common_ip_block);
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
  pthread_mutex_destroy(&adev->gpu_lock);
}

// Just asking the GPU "Who are you?"
int amdgpu_gpu_get_info_hal(struct OBJGPU *adev, amdgpu_gpu_info_t *info) {
  os_prim_log("HAL: [Manager] Giving out the GPU ID card.\n");

  if (adev->asic_type == AMD_ASIC_WRESTLER) {
    info->vram_size_mb = 512; // Typical for this APU
    info->gpu_clock_mhz = 400;
    strncpy(info->gpu_name, "Radeon HD 7290 (Wrestler)", 31);
  } else {
    info->vram_size_mb = 8192;
    info->gpu_clock_mhz = 1710;
    strncpy(info->gpu_name, "Radeon RX 5700 XT (Abstracted)", 31);
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
  // Imagine cool 3D graphics happening here!
  return 0;
}
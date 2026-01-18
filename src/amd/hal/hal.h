#ifndef AMD_HAL_H
#define AMD_HAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Haiku: Include display mode definitions early */
#ifdef __HAIKU__
/* Try multiple include paths for GraphicsDefs.h */
#if __has_include(<graphics/GraphicsDefs.h>)
  #include <graphics/GraphicsDefs.h>
#elif __has_include(<GraphicsDefs.h>)
  #include <GraphicsDefs.h>
#else
  /* Fallback: define minimal display_mode if header not found */
  /* This allows compilation even if GraphicsDefs.h is not available */
  #ifndef _DISPLAY_MODE_H
    typedef struct {
      uint16_t width;
      uint16_t height;
      uint16_t virtual_width;
      uint16_t virtual_height;
      uint32_t space;
      uint32_t pixel_clock;
      uint32_t h_sync_start;
      uint32_t h_sync_end;
      uint32_t h_total;
      uint32_t v_sync_start;
      uint32_t v_sync_end;
      uint32_t v_total;
      uint32_t flags;
    } display_mode;
  #endif
#endif
#endif

/*
 * Yo! This is the Hardware Abstraction Layer (HAL) for AMD GPUs.
 * We took some cool ideas from the pro Linux and NVIDIA drivers and
 * remixed them to be super easy to use.
 *
 * Think of this as the "Universal Translator" between your code and the
 * hardware. Developed with heart by: Haiku Imposible Team (HIT)
 */

#define AMDGPU_MAX_IP_BLOCKS 16

struct OBJGPU;

// Cool macro to keep track of versions (like 1.0.2!)
#define IP_VERSION(maj, min, rev) (((maj) << 16) | ((min) << 8) | (rev))

// Different types of "Builders" (IP Blocks) inside the GPU
enum amd_ip_block_type {
  AMD_IP_BLOCK_TYPE_COMMON, // General stuff
  AMD_IP_BLOCK_TYPE_GMC,    // The "Memory Manager" specialist
  AMD_IP_BLOCK_TYPE_IH,     // The "Interrupt" specialist
  AMD_IP_BLOCK_TYPE_PSP,    // Security specialist
  AMD_IP_BLOCK_TYPE_SMC,    // Power/Speed specialist
  AMD_IP_BLOCK_TYPE_GFX,    // The "3D Graphics" specialist
  AMD_IP_BLOCK_TYPE_SDMA,   // High-speed data mover
  AMD_IP_BLOCK_TYPE_VCN,    // Video specialist
  AMD_IP_BLOCK_TYPE_DCE,    // The "Display/Monitor" specialist
};

// The skills every specialist worker must have
struct amd_ip_funcs {
  const char *name;
  int (*early_init)(struct OBJGPU *adev);    // Fast check
  int (*sw_init)(struct OBJGPU *adev);       // Setting up software
  int (*hw_init)(struct OBJGPU *adev);       // Turning on hardware
  int (*late_init)(struct OBJGPU *adev);     // Finishing touches
  int (*hw_fini)(struct OBJGPU *adev);       // Turning it off cleanly
  int (*suspend)(struct OBJGPU *adev);       // Going to sleep
  int (*resume)(struct OBJGPU *adev);        // Waking up
  bool (*is_idle)(struct OBJGPU *adev);      // Are you doing anything?
  int (*wait_for_idle)(struct OBJGPU *adev); // Please wait until done
  int (*soft_reset)(struct OBJGPU *adev);    // Turn it off and on again!
  void (*set_clockgating_state)(struct OBJGPU *adev, int state);
  void (*set_powergating_state)(struct OBJGPU *adev, int state);
};

// The blueprint for a specialist
struct amd_ip_block_version {
  enum amd_ip_block_type type;
  uint32_t major;
  uint32_t minor;
  uint32_t rev;
  const struct amd_ip_funcs *funcs;
};

// A specialist worker currently on the job
struct amd_ip_block {
  const struct amd_ip_block_version *version;
  bool status; // true if they are ready to work!
};

// The "Tagging System" (RESSERV) to keep track of everything we created
struct RsResource {
  uint32_t handle;               // A unique ID (like a name tag)
  struct RsResource *parent;     // Who created me?
  struct RsResource *child_list; // First child in my family tree
  struct RsResource *sibling;    // My brother/sister in the family tree
  struct RsResource *hash_next;  // Next resource in the global lookup table
  void *data;                    // The actual stuff (like memory)
  /* Synchronization handled by OS primitives if needed */
};

struct RsResource *rs_resource_create(uint32_t handle,
                                      struct RsResource *parent);
void rs_resource_add_child(struct RsResource *parent, struct RsResource *child);
struct RsResource *rs_resource_lookup(uint32_t handle);
void rs_resource_destroy(struct RsResource *res);

// GPU Memory Buffers and Command Lists
struct amdgpu_buffer {
  void *cpu_addr;    // Where the CPU sees it
  uint64_t gpu_addr; // Where the GPU sees it
  size_t size;       // How big is it?
};

struct amdgpu_command_buffer {
  void *cmds; // The list of things to do
  size_t size;
};

// Basic info about your cool GPU
struct amdgpu_gpu_info {
  uint32_t vram_size_mb;
  uint32_t gpu_clock_mhz;
  char gpu_name[32];
  uint64_t vram_base; // Physical address of VRAM (for Accelerant)
};

typedef struct amdgpu_gpu_info amdgpu_gpu_info_t;

// Different ASIC types
enum amd_asic_type {
  AMD_ASIC_WRESTLER,  // The APU specialist
  AMD_ASIC_NAVI10,    // The modern powerhouse
  AMD_ASIC_R600,      // Legacy HD 2000-4000
  AMD_ASIC_EVERGREEN, // Legacy HD 5000-6000
  AMD_ASIC_NI,        // Legacy Northern Islands
};

// --- The Belter "Shadow State" ---
// Mirrors critical registers in RAM for "Self-Healing"
struct amd_shadow_state {
  uint32_t regs[1024]; // Simple shadow of first 1K MMIO registers
  bool valid[1024];    // Did we write to this register?
};

// GPU State Flags for "Heartbeat"
enum amd_gpu_state {
  AMD_GPU_STATE_RUNNING = 0,
  AMD_GPU_STATE_HUNG,
  AMD_GPU_STATE_RESETTING,
};

// The "Main Brain" (OBJGPU) that manages all our specialists
struct OBJGPU {
  enum amd_asic_type asic_type; // What kind of GPU chip is this?
  uint16_t device_id;           // The specific PCI ID
  void *pci_handle;             // The connection to the OS PCI bus
  uint32_t family;              // Which GPU family does it belong to?

  struct amd_ip_block ip_blocks[AMDGPU_MAX_IP_BLOCKS]; // List of workers
  int num_ip_blocks;

  void *mmio_base;             // The direct connection to the hardware
  struct RsResource *res_root; // The top of the "Family Tree"

  // GPU capabilities and memory info
  struct amdgpu_gpu_info gpu_info;  // Cached GPU info (VRAM base, size, clock)

  /* Synchronization via OS primitives (os_prim_lock/unlock) */

  // Belter Strategy: Resilience Layer
  struct amd_shadow_state shadow;
  enum amd_gpu_state state;
  /* Thread handling via os_prim_spawn_thread */
};

// The HAL API: The main commands you will use!
int amdgpu_device_init_hal(struct OBJGPU *adev);
void amdgpu_device_fini_hal(struct OBJGPU *adev);
int amdgpu_gpu_get_info_hal(struct OBJGPU *adev, amdgpu_gpu_info_t *info);
int amdgpu_buffer_alloc_hal(struct OBJGPU *adev, size_t size,
                            struct amdgpu_buffer *buf);
void amdgpu_buffer_free_hal(struct OBJGPU *adev, struct amdgpu_buffer *buf);
int amdgpu_command_submit_hal(struct OBJGPU *adev,
                              struct amdgpu_command_buffer *cb);

// Display Mode Setting - disabled for now due to header compatibility issues
// int amdgpu_set_display_mode_hal(struct OBJGPU *adev, const struct display_mode *mode);

// Memory Controller Scanout Address (Phase 2.2)
int gmc_v10_set_scanout_address(struct OBJGPU *adev, uint64_t gpu_address);

// Pixel Clock Control (Phase 2.3)
int clock_v10_set_pixel_clock(struct OBJGPU *adev, uint32_t pixel_clock_10khz);
uint32_t clock_v10_get_pixel_clock(struct OBJGPU *adev);
int clock_v10_disable_clock(struct OBJGPU *adev);

// Helping the brain find new specialists
int amdgpu_device_ip_block_add(
    struct OBJGPU *adev, const struct amd_ip_block_version *ip_block_version);

// Belter "Self-Healing" API
void amdgpu_hal_shadow_write(struct OBJGPU *adev, uint32_t offset,
                             uint32_t value);
int amdgpu_hal_reset(struct OBJGPU *adev);
void *amdgpu_hal_heartbeat(void *arg);

#endif /* AMD_HAL_H */
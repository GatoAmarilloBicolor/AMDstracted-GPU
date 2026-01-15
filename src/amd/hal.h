#ifndef AMD_HAL_H
#define AMD_HAL_H

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Hardware Abstraction Layer for AMD GPUs, inspired by drm-kmod and NVIDIA
// Uses modular IP Block architecture for true hardware abstraction
// Maintained by: Haiku Imposible Team

#define AMDGPU_MAX_IP_BLOCKS 16

struct OBJGPU;

// IP Block Versioning
#define IP_VERSION(maj, min, rev) (((maj) << 16) | ((min) << 8) | (rev))

enum amd_ip_block_type {
  AMD_IP_BLOCK_TYPE_COMMON,
  AMD_IP_BLOCK_TYPE_GMC,
  AMD_IP_BLOCK_TYPE_IH,
  AMD_IP_BLOCK_TYPE_PSP,
  AMD_IP_BLOCK_TYPE_SMC,
  AMD_IP_BLOCK_TYPE_GFX,
  AMD_IP_BLOCK_TYPE_SDMA,
  AMD_IP_BLOCK_TYPE_VCN,
  AMD_IP_BLOCK_TYPE_DCE,
};

// Modular IP functions (inspired by amd_ip_funcs in drm-kmod)
struct amd_ip_funcs {
  const char *name;
  int (*early_init)(struct OBJGPU *adev);
  int (*sw_init)(struct OBJGPU *adev);
  int (*hw_init)(struct OBJGPU *adev);
  int (*late_init)(struct OBJGPU *adev);
  int (*hw_fini)(struct OBJGPU *adev);
  int (*suspend)(struct OBJGPU *adev);
  int (*resume)(struct OBJGPU *adev);
  bool (*is_idle)(struct OBJGPU *adev);
  int (*wait_for_idle)(struct OBJGPU *adev);
  int (*soft_reset)(struct OBJGPU *adev);
  void (*set_clockgating_state)(struct OBJGPU *adev, int state);
  void (*set_powergating_state)(struct OBJGPU *adev, int state);
};

struct amd_ip_block_version {
  enum amd_ip_block_type type;
  uint32_t major;
  uint32_t minor;
  uint32_t rev;
  const struct amd_ip_funcs *funcs;
};

struct amd_ip_block {
  const struct amd_ip_block_version *version;
  bool status; // true if initialized
};

// Existing Resource Management structures (kept for compatibility/hierarchy)
struct RsResource {
  uint32_t handle;
  struct RsResource *parent;
  struct RsResource **children;
  int num_children;
  void *data;
  pthread_mutex_t lock;
};

struct RsResource *rs_resource_create(uint32_t handle,
                                      struct RsResource *parent);
void rs_resource_add_child(struct RsResource *parent, struct RsResource *child);
void rs_resource_destroy(struct RsResource *res);

// Buffer and Command structures
struct amdgpu_buffer {
  void *cpu_addr;
  uint64_t gpu_addr;
  size_t size;
};

struct amdgpu_command_buffer {
  void *cmds;
  size_t size;
};

struct amdgpu_gpu_info {
  uint32_t vram_size_mb;
  uint32_t gpu_clock_mhz;
  char gpu_name[32];
};

typedef struct amdgpu_gpu_info amdgpu_gpu_info_t;

// The main GPU object now manages a list of IP Blocks
struct OBJGPU {
  uint32_t asic_type;
  uint32_t family;

  struct amd_ip_block ip_blocks[AMDGPU_MAX_IP_BLOCKS];
  int num_ip_blocks;

  void *mmio_base;
  struct RsResource *res_root; // Root of RESSERV hierarchy

  pthread_mutex_t gpu_lock;
};

// HAL API
int amdgpu_device_init_hal(struct OBJGPU *adev);
void amdgpu_device_fini_hal(struct OBJGPU *adev);
int amdgpu_gpu_get_info_hal(struct OBJGPU *adev, amdgpu_gpu_info_t *info);
int amdgpu_buffer_alloc_hal(struct OBJGPU *adev, size_t size,
                            struct amdgpu_buffer *buf);
void amdgpu_buffer_free_hal(struct OBJGPU *adev, struct amdgpu_buffer *buf);
int amdgpu_command_submit_hal(struct OBJGPU *adev,
                              struct amdgpu_command_buffer *cb);

// IP Block Management
int amdgpu_device_ip_block_add(
    struct OBJGPU *adev, const struct amd_ip_block_version *ip_block_version);

#endif // AMD_HAL_H
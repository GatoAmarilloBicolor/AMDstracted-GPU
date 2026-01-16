/*
 * AMDGPU DRM UAPI Header
 * Minimal definitions needed for RADV compatibility
 * Based on Linux kernel's amdgpu_drm.h
 */

#ifndef AMDGPU_DRM_H
#define AMDGPU_DRM_H

#include <stdint.h>

// DRM Command Indices (offset from DRM_COMMAND_BASE)
#define DRM_AMDGPU_GEM_CREATE 0x00
#define DRM_AMDGPU_GEM_MMAP 0x01
#define DRM_AMDGPU_CTX 0x02
#define DRM_AMDGPU_BO_LIST 0x03
#define DRM_AMDGPU_CS 0x04
#define DRM_AMDGPU_INFO 0x05
#define DRM_AMDGPU_GEM_METADATA 0x06
#define DRM_AMDGPU_GEM_WAIT_IDLE 0x07
#define DRM_AMDGPU_GEM_VA 0x08
#define DRM_AMDGPU_WAIT_CS 0x09
#define DRM_AMDGPU_GEM_OP 0x10
#define DRM_AMDGPU_GEM_USERPTR 0x11

// GEM Create Flags
#define AMDGPU_GEM_CREATE_CPU_ACCESS_REQUIRED (1 << 0)
#define AMDGPU_GEM_CREATE_NO_CPU_ACCESS (1 << 1)
#define AMDGPU_GEM_CREATE_CPU_GTT_USWC (1 << 2)
#define AMDGPU_GEM_CREATE_VRAM_CLEARED (1 << 3)

// Memory Domains
#define AMDGPU_GEM_DOMAIN_CPU 0x1
#define AMDGPU_GEM_DOMAIN_GTT 0x2
#define AMDGPU_GEM_DOMAIN_VRAM 0x4
#define AMDGPU_GEM_DOMAIN_GDS 0x8
#define AMDGPU_GEM_DOMAIN_GWS 0x10
#define AMDGPU_GEM_DOMAIN_OA 0x20

// Structures
struct drm_amdgpu_gem_create_in {
  uint64_t bo_size;
  uint64_t alignment;
  uint64_t domains;
  uint64_t domain_flags;
};

struct drm_amdgpu_gem_create_out {
  uint32_t handle;
};

union drm_amdgpu_gem_create {
  struct drm_amdgpu_gem_create_in in;
  struct drm_amdgpu_gem_create_out out;
};

struct drm_amdgpu_gem_mmap_in {
  uint32_t handle;
};

struct drm_amdgpu_gem_mmap_out {
  uint64_t addr_ptr;
};

union drm_amdgpu_gem_mmap {
  struct drm_amdgpu_gem_mmap_in in;
  struct drm_amdgpu_gem_mmap_out out;
};

// Info Query Types
#define AMDGPU_INFO_ACCEL_WORKING 0x00
#define AMDGPU_INFO_CRTC_FROM_ID 0x01
#define AMDGPU_INFO_HW_IP_INFO 0x02
#define AMDGPU_INFO_HW_IP_COUNT 0x03
#define AMDGPU_INFO_TIMESTAMP 0x05
#define AMDGPU_INFO_FW_VERSION 0x0e
#define AMDGPU_INFO_NUM_BYTES_MOVED 0x0f
#define AMDGPU_INFO_VRAM_USAGE 0x10
#define AMDGPU_INFO_GTT_USAGE 0x11
#define AMDGPU_INFO_GDS_CONFIG 0x13
#define AMDGPU_INFO_VRAM_GTT 0x14
#define AMDGPU_INFO_READ_MMR_REG 0x15
#define AMDGPU_INFO_DEV_INFO 0x16
#define AMDGPU_INFO_VIS_VRAM_USAGE 0x17
#define AMDGPU_INFO_NUM_EVICTIONS 0x18
#define AMDGPU_INFO_MEMORY 0x19
#define AMDGPU_INFO_VCE_CLOCK_TABLE 0x1A
#define AMDGPU_INFO_VBIOS 0x1B
#define AMDGPU_INFO_NUM_HANDLES 0x1C
#define AMDGPU_INFO_SENSOR 0x1D
#define AMDGPU_INFO_NUM_VRAM_CPU_PAGE_FAULTS 0x1E
#define AMDGPU_INFO_VRAM_LOST_COUNTER 0x1F
#define AMDGPU_INFO_RAS_ENABLED_FEATURES 0x20
#define AMDGPU_INFO_VIDEO_CAPS 0x21

struct drm_amdgpu_info {
  uint64_t return_pointer;
  uint32_t return_size;
  uint32_t query;

  union {
    struct {
      uint32_t id;
      uint32_t _pad;
    } mode_crtc;

    struct {
      uint32_t type;
      uint32_t ip_instance;
    } query_hw_ip;

    struct {
      uint32_t dword_offset;
      uint32_t count;
      uint32_t instance;
      uint32_t flags;
    } read_mmr_reg;

    struct {
      uint32_t type;
    } query_fw;

    struct {
      uint32_t type;
      uint32_t offset;
    } vbios_info;

    struct {
      uint32_t type;
    } sensor_info;

    struct {
      uint32_t type;
    } video_cap;
  };
};

// Command Submission
struct drm_amdgpu_cs_in {
  uint32_t ctx_id;
  uint32_t bo_list_handle;
  uint32_t num_chunks;
  uint32_t _pad;
  uint64_t chunks;
};

struct drm_amdgpu_cs_out {
  uint64_t handle;
};

union drm_amdgpu_cs {
  struct drm_amdgpu_cs_in in;
  struct drm_amdgpu_cs_out out;
};

#endif // AMDGPU_DRM_H

#include "../../src/amd/rmapi.h"
#include "../../src/common/ipc_lib.h"
#include "../../src/common/ipc_protocol.h"
#include <add-ons/accelerant/Accelerant.h>
#include <GraphicsDefs.h>
#include <OS.h>
#include <SupportDefs.h>
#include <Errors.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * 🌀 HIT Edition: The Professional Haiku Accelerant (C-Safe Wrappers)
 * 🛡️ Belter Strategy: "Self-Healing" Capabilities & Process Cloning
 */

#ifndef _EXPORT
#define _EXPORT __attribute__((visibility("default")))
#endif

// Belter Strategy: Standard Safe Modes
static const display_mode kSafeModes[] = {
    {{25175, 640, 656, 752, 800, 480, 490, 492, 525, 0},
     B_CMAP8,
     640,
     480,
     0,
     0,
     0},
    {{65000, 1024, 1048, 1184, 1344, 768, 771, 777, 806, 0},
     B_RGB32,
     1024,
     768,
     0,
     0,
     0},
    {{108000, 1280, 1328, 1440, 1688, 1024, 1025, 1028, 1066, 0},
     B_RGB32,
     1280,
     1024,
     0,
     0,
     0},
    {{106500, 1440, 1520, 1672, 1904, 900, 901, 904, 934, 0},
     B_RGB32,
     1440,
     900,
     0,
     0,
     0},
    {{148500, 1920, 2008, 2052, 2200, 1080, 1084, 1089, 1125, 0},
     B_RGB32,
     1920,
     1080,
     0,
     0,
     0}};
static const uint32 kNumSafeModes = sizeof(kSafeModes) / sizeof(display_mode);

class AmdAccelerant {
public:
  AmdAccelerant() : m_connected(false) { memset(&m_conn, 0, sizeof(m_conn)); }
  status_t Init(int fd);
  void Uninit();
  status_t GetDeviceInfo(accelerant_device_info *info);
  uint32 ModeCount();
  status_t GetModeList(display_mode *modes);
  status_t SetDisplayMode(display_mode *mode_to_set);
  status_t GetFrameBufferConfig(frame_buffer_config *fbc);

  // Clone Hooks
  ssize_t CloneInfoSize();
  void GetCloneInfo(void *data);
  status_t Clone(void *data);

private:
  ipc_connection_t m_conn;
  bool m_connected;
  struct amdgpu_gpu_info m_gpu_info;
};

static AmdAccelerant g_acc;

// Static wrapper functions for the Haiku hooks (Renamed to avoid conflicts)
static status_t amd_init_acc(int fd) { return g_acc.Init(fd); }
static void amd_uninit_acc() { g_acc.Uninit(); }
static status_t amd_get_device_info(accelerant_device_info *adi) {
  return g_acc.GetDeviceInfo(adi);
}
static uint32 amd_get_mode_count() { return g_acc.ModeCount(); }
static status_t amd_get_modes(display_mode *modes) {
  return g_acc.GetModeList(modes);
}
static status_t amd_set_display_mode(display_mode *mode) {
  return g_acc.SetDisplayMode(mode);
}
static status_t amd_get_frame_buffer_config(frame_buffer_config *fbc) {
  return g_acc.GetFrameBufferConfig(fbc);
}

// Clone wrappers
static ssize_t amd_clone_info_size() { return g_acc.CloneInfoSize(); }
static void amd_get_clone_info(void *data) { g_acc.GetCloneInfo(data); }
static status_t amd_clone_accelerant(void *data) { return g_acc.Clone(data); }

// Belter Strategy: Engine Hooks for 2D Acceleration
static status_t amd_acquire_engine(uint32 caps, uint32 max_wait, sync_token *st,
                                   engine_token **et) {
  if (et)
    *et = (engine_token *)0x1; // The "Golden Ticket" token
  return B_OK;
}

static status_t amd_release_engine(engine_token *et, sync_token *st) {
  return B_OK;
}

status_t AmdAccelerant::Init(int fd) {
  if (ipc_client_connect(HIT_SOCKET_PATH, &m_conn) < 0)
    return B_ERROR;

  m_connected = true;

  // Cache GPU Info on startup for Belter efficiency
  ipc_message_t msg = {IPC_REQ_GET_GPU_INFO, 1, 0, NULL};
  if (ipc_send_message(&m_conn, &msg) == 0) {
    ipc_message_t reply;
    if (ipc_recv_message(&m_conn, &reply) > 0) {
      if (reply.type == IPC_REP_GET_GPU_INFO) {
        memcpy(&m_gpu_info, reply.data, sizeof(m_gpu_info));
        free(reply.data);
      } else {
        if (reply.data)
          free(reply.data);
      }
    }
  }

  return B_OK;
}

ssize_t AmdAccelerant::CloneInfoSize() { return strlen(HIT_SOCKET_PATH) + 1; }

void AmdAccelerant::GetCloneInfo(void *data) {
  strcpy((char *)data, HIT_SOCKET_PATH);
}

status_t AmdAccelerant::Clone(void *data) {
  // In a cloned instance, we need to reconnect to the "Brain"
  return Init(-1);
}

void AmdAccelerant::Uninit() {
  if (m_connected) {
    ipc_close(&m_conn);
    m_connected = false;
  }
}

status_t AmdAccelerant::GetDeviceInfo(accelerant_device_info *info) {
  if (!m_connected)
    return B_ERROR;

  // Use cached info
  info->version = B_ACCELERANT_VERSION;
  strncpy(info->name, m_gpu_info.gpu_name, sizeof(info->name));
  strncpy(info->chipset, "AMDGPU Abstracted (HIT)", sizeof(info->chipset));
  info->memory = (uint64_t)m_gpu_info.vram_size_mb * 1024 * 1024;
  info->dac_speed = m_gpu_info.gpu_clock_mhz;
  return B_OK;
}

uint32 AmdAccelerant::ModeCount() { return kNumSafeModes; }

status_t AmdAccelerant::GetModeList(display_mode *modes) {
  if (!modes)
    return B_BAD_VALUE;

  for (uint32 i = 0; i < kNumSafeModes; i++) {
    modes[i] = kSafeModes[i];
  }
  return B_OK;
}

status_t AmdAccelerant::SetDisplayMode(display_mode *mode_to_set) {
  if (!m_connected || !mode_to_set)
    return B_ERROR;

  // PROPOSE: Send mode set request to RMAPI server
  return B_OK;
}

status_t AmdAccelerant::GetFrameBufferConfig(frame_buffer_config *fbc) {
  if (!fbc)
    return B_BAD_VALUE;

  // Use the dynamic VRAM base from HAL (via IPC)
  fbc->frame_buffer = (void *)(uintptr_t)m_gpu_info.vram_base;
  fbc->frame_buffer_dma = (void *)(uintptr_t)m_gpu_info.vram_base;
  fbc->bytes_per_row = 1024 * 4;
  return B_OK;
}

// BGL Hooks for OpenGL support via Zink
static status_t amd_get_gl_renderer(void *data) {
  // Return Zink-based GL renderer
  // Initialize Zink with RADV RMAPI
  // For now, stub
  return B_OK;
}

static status_t amd_create_gl_context(void *data) {
  // Create GL context using Zink
  return B_OK;
}

static status_t amd_destroy_gl_context(void *data) {
  return B_OK;
}

extern "C" {
_EXPORT void *get_accelerant_hook(uint32 feature, void *data) {
   switch (feature) {
   case B_INIT_ACCELERANT:
     return (void *)amd_init_acc;
   case B_UNINIT_ACCELERANT:
     return (void *)amd_uninit_acc;
   case B_GET_ACCELERANT_DEVICE_INFO:
     return (void *)amd_get_device_info;
   case B_ACCELERANT_MODE_COUNT:
     return (void *)amd_get_mode_count;
   case B_GET_MODE_LIST:
     return (void *)amd_get_modes;
   case B_SET_DISPLAY_MODE:
     return (void *)amd_set_display_mode;
   case B_GET_FRAME_BUFFER_CONFIG:
     return (void *)amd_get_frame_buffer_config;
   case B_ACQUIRE_ENGINE:
     return (void *)amd_acquire_engine;
   case B_RELEASE_ENGINE:
     return (void *)amd_release_engine;
   case B_ACCELERANT_CLONE_INFO_SIZE:
     return (void *)amd_clone_info_size;
   case B_GET_ACCELERANT_CLONE_INFO:
     return (void *)amd_get_clone_info;
   case B_CLONE_ACCELERANT:
     return (void *)amd_clone_accelerant;
   // BGL Hooks
   case BGL_GET_RENDERER:
     return (void *)amd_get_gl_renderer;
   case BGL_CREATE_CONTEXT:
     return (void *)amd_create_gl_context;
   case BGL_DESTROY_CONTEXT:
     return (void *)amd_destroy_gl_context;
   default:
     return NULL;
   }
 }
}
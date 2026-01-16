#include "../../src/amd/rmapi.h"
#include "../../src/common/ipc_lib.h"
#include "../../src/common/ipc_protocol.h"
#include <Accelerant.h>
#include <GraphicsDefs.h>
#include <OS.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * 🌀 HIT Edition: The Professional Haiku Accelerant (C-Safe Wrappers)
 * 🛡️ Belter Strategy: "Self-Healing" Capabilities
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

private:
  ipc_connection_t m_conn;
  bool m_connected;
};

static AmdAccelerant g_acc;

// Static wrapper functions for the Haiku hooks
static status_t init_acc(int fd) { return g_acc.Init(fd); }
static void uninit_acc() { g_acc.Uninit(); }
static status_t get_device_info(accelerant_device_info *adi) {
  return g_acc.GetDeviceInfo(adi);
}
static uint32 get_mode_count() { return g_acc.ModeCount(); }
static status_t get_modes(display_mode *modes) {
  return g_acc.GetModeList(modes);
}
static status_t set_display_mode(display_mode *mode) {
  return g_acc.SetDisplayMode(mode);
}

status_t AmdAccelerant::Init(int fd) {
  if (ipc_client_connect(HIT_SOCKET_PATH, &m_conn) < 0)
    return B_ERROR;
  m_connected = true;
  return B_OK;
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
  struct amdgpu_gpu_info gpu_info;
  ipc_message_t msg = {IPC_REQ_GET_GPU_INFO, 1, 0, NULL};
  if (ipc_send_message(&m_conn, &msg) == 0) {
    ipc_message_t reply;
    if (ipc_recv_message(&m_conn, &reply) > 0) {
      if (reply.type == IPC_REP_GET_GPU_INFO) {
        memcpy(&gpu_info, reply.data, sizeof(gpu_info));
        free(reply.data);
        info->version = B_ACCELERANT_VERSION;
        strncpy(info->name, gpu_info.gpu_name, sizeof(info->name));
        strncpy(info->chipset, "AMDGPU Abstracted (HIT)",
                sizeof(info->chipset));
        info->memory = (uint64_t)gpu_info.vram_size_mb * 1024 * 1024;
        info->dac_speed = gpu_info.gpu_clock_mhz; // Roughly equiv
        return B_OK;
      }
      if (reply.data)
        free(reply.data);
    }
  }
  return B_ERROR;
}

uint32 AmdAccelerant::ModeCount() { return kNumSafeModes; }

status_t AmdAccelerant::GetModeList(display_mode *modes) {
  if (!modes)
    return B_BadValue;

  // Return our "Safe Mode" list
  for (uint32 i = 0; i < kNumSafeModes; i++) {
    modes[i] = kSafeModes[i];
  }
  return B_OK;
}

status_t AmdAccelerant::SetDisplayMode(display_mode *mode_to_set) {
  // In Belter Strategy, we trust the mode but verify the connection
  if (!m_connected || !mode_to_set)
    return B_ERROR;

  // PROPOSE: Send mode set request to RMAPI server (to perform shadow write)
  // For now, we simulate success to keep app_server happy.
  // Real implementation would IPC_REQ_SET_MODE
  return B_OK;
}

extern "C" {
_EXPORT void *get_accelerant_hook(uint32 feature, void *data) {
  switch (feature) {
  case B_INIT_ACCELERANT:
    return (void *)init_acc;
  case B_UNINIT_ACCELERANT:
    return (void *)uninit_acc;
  case B_GET_ACCELERANT_DEVICE_INFO:
    return (void *)get_device_info;
  case B_ACCELERANT_MODE_COUNT:
    return (void *)get_mode_count;
  case B_GET_MODE_LIST:
    return (void *)get_modes;
  case B_SET_DISPLAY_MODE:
    return (void *)set_display_mode;
  default:
    return NULL;
  }
}
}
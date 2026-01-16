#include "../../src/amd/rmapi.h"
#include "../../src/common/ipc_lib.h"
#include <Accelerant.h>
#include <GraphicsDefs.h>
#include <OS.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * 🌀 HIT Edition: The Professional Haiku Accelerant (C-Safe Wrappers)
 */

#ifndef _EXPORT
#define _EXPORT __attribute__((visibility("default")))
#endif

class AmdAccelerant {
public:
  AmdAccelerant() : m_connected(false) { memset(&m_conn, 0, sizeof(m_conn)); }
  status_t Init(int fd);
  void Uninit();
  status_t GetDeviceInfo(accelerant_device_info *info);
  uint32 ModeCount() { return 1; }
  status_t GetModeList(display_mode *modes);

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

status_t AmdAccelerant::Init(int fd) {
  if (ipc_client_connect("/tmp/amdgpu_hit.sock", &m_conn) < 0)
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
  ipc_message_t msg = {3, 101, 0, NULL};
  if (ipc_send_message(&m_conn, &msg) == 0) {
    ipc_message_t reply;
    if (ipc_recv_message(&m_conn, &reply) > 0) {
      memcpy(&gpu_info, reply.data, sizeof(gpu_info));
      free(reply.data);
      info->version = B_ACCELERANT_VERSION;
      strncpy(info->name, gpu_info.gpu_name, sizeof(info->name));
      strncpy(info->chipset, "AMDGPU Abstracted (HIT)", sizeof(info->chipset));
      info->memory = (uint64_t)gpu_info.vram_size_mb * 1024 * 1024;
      return B_OK;
    }
  }
  return B_ERROR;
}

status_t AmdAccelerant::GetModeList(display_mode *modes) {
  modes[0].timing.pixel_clock = 10000;
  modes[0].timing.h_display = 1024;
  modes[0].timing.v_display = 768;
  modes[0].space = B_RGB32;
  modes[0].virtual_width = 1024;
  modes[0].virtual_height = 768;
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
  default:
    return NULL;
  }
}
}
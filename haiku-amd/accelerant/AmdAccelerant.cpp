#include "../../src/amd/rmapi.h"
#include "../../src/common/ipc_lib.h"
#include <Accelerant.h>
#include <GraphicsDefs.h>
#include <OS.h>
#include <stdio.h>
#include <string.h>

/*
 * 🌀 HIT Edition: The Real Haiku Accelerant (Proxy Mode)
 *
 * This is the "face" of our driver for Haiku. It talks to the RMAPI Server
 * (The Brain) to get stuff done. It implements the official Haiku
 * accelerant API so things like GLInfo and Mesa can finally work!
 */

class AmdAccelerant {
public:
  AmdAccelerant() : m_connected(false) { memset(&m_conn, 0, sizeof(m_conn)); }

  status_t Init();
  void Uninit();

  // Real Info Discovery
  status_t GetDeviceInfo(accelerant_device_info *info);
  status_t GetModeList(display_mode *modes);

private:
  ipc_connection_t m_conn;
  bool m_connected;
};

// Start talking to the RMAPI Brain!
status_t AmdAccelerant::Init() {
  if (ipc_client_connect("/tmp/amdgpu_rmapi.sock", &m_conn) < 0) {
    return B_ERROR;
  }
  m_connected = true;
  return B_OK;
}

void AmdAccelerant::Uninit() {
  if (m_connected) {
    ipc_close(&m_conn);
    m_connected = false;
  }
}

// Responding to Haiku's "Tell me about this GPU" request
status_t AmdAccelerant::GetDeviceInfo(accelerant_device_info *info) {
  if (!m_connected)
    return B_ERROR;

  struct amdgpu_gpu_info gpu_info;
  ipc_message_t msg = {3, 101, 0, NULL}; // IPC_GET_GPU_INFO

  if (ipc_send_message(&m_conn, &msg) == 0) {
    ipc_message_t reply;
    if (ipc_recv_message(&m_conn, &reply) > 0) {
      memcpy(&gpu_info, reply.data, sizeof(gpu_info));
      free(reply.data);

      info->version = 1;
      strncpy(info->name, gpu_info.gpu_name, 31);
      strncpy(info->chipset, "AMDGPU Abstracted (HIT)", 31);
      info->memory = gpu_info.vram_size_mb * 1024 * 1024;
      return B_OK;
    }
  }
  return B_ERROR;
}

// Standard Haiku Entry Point for Accelerants
extern "C" status_t get_accelerant_hook(uint32 feature, void **hook) {
  static AmdAccelerant acc;
  static bool initialized = false;

  if (!initialized) {
    if (acc.Init() == B_OK)
      initialized = true;
  }

  switch (feature) {
  case B_ACCELERANT_DEVICE_INFO:
    *hook = (void *)&acc.GetDeviceInfo;
    return B_OK;
    // Add more hooks as we implement the "Muscle" (GFX/Compute)
  }

  return B_ERROR;
}
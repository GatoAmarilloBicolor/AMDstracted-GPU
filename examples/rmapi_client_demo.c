#include "../core/rmapi/rmapi.h"
#include "../core/ipc/ipc_lib.h"
#include "../core/ipc/ipc_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * 🌀 HIT Edition: RMAPI Client Demo
 *
 * This little app demonstrates how to talk to our consolidated
 * RMAPI server using the new HIT Subway signals (IPC protocol).
 */

int main() {
  printf("🌀 HIT Client: Connecting to the GPU Subway (%s)...\n",
         HIT_SOCKET_PATH);

  ipc_connection_t conn;
  if (ipc_client_connect(HIT_SOCKET_PATH, &conn) < 0) {
    perror("❌ Connection failed! Is the amd_rmapi_server running?");
    return 1;
  }
  printf("✅ Connected to the Driver Brain!\n");

  // 1. Request GPU Info
  printf("📡 Sending Request: GET_GPU_INFO...\n");
  ipc_message_t msg = {IPC_REQ_GET_GPU_INFO, 1, 0, NULL};
  if (ipc_send_message(&conn, &msg) < 0) {
    printf("❌ Failed to send request.\n");
    ipc_close(&conn);
    return 1;
  }

  // 2. Wait for Reply
  ipc_message_t resp;
  if (ipc_recv_message(&conn, &resp) > 0) {
    if (resp.type == IPC_REP_GET_GPU_INFO) {
      struct amdgpu_gpu_info *info = (struct amdgpu_gpu_info *)resp.data;
      printf("----------------------------------------------------\n");
      printf("🎮 GPU Identity Confirmed!\n");
      printf("📍 Name:   %s\n", info->gpu_name);
      printf("📍 VRAM:   %u MB\n", info->vram_size_mb);
      printf("📍 Clock:  %u MHz\n", info->gpu_clock_mhz);
      printf("----------------------------------------------------\n");
      free(resp.data);
    } else {
      printf("⚠️ Received unexpected message type: %u\n", resp.type);
    }
  }

  printf("👋 Closing connection. HIT Edition rules!\n");
  ipc_close(&conn);
  return 0;
}
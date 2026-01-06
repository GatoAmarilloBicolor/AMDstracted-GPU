// Cliente demo para RMAPI server
#include "src/common/ipc_lib.h"
#include "src/amd/rmapi.h"
#include <stdio.h>

int main() {
    printf("Connecting to RMAPI server...\n");
    ipc_connection_t conn;
    if (ipc_client_connect("/tmp/amdgpu_rmapi.sock", &conn) < 0) {
        perror("Connect failed");
        return 1;
    }
    printf("Connected!\n");

    // Request GPU info
    ipc_send_message(&conn, &(ipc_message_t){3, 1, 0, NULL});  // IPC_GET_GPU_INFO

    ipc_message_t resp;
    ipc_recv_message(&conn, &resp);
    struct amdgpu_gpu_info* info = (struct amdgpu_gpu_info*)resp.data;
    printf("GPU Info from Server: %s, %u MB VRAM\n", info->gpu_name, info->vram_size_mb);

    ipc_close(&conn);
    return 0;
}
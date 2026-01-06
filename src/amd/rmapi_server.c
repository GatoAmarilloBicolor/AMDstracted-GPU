#include "../common/ipc_lib.h"
#include "hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>

// RMAPI Server: Handles IPC requests for GPU operations
// Optimized with threading and caching

#define SOCKET_PATH "/tmp/amdgpu_rmapi.sock"

typedef struct {
    ipc_connection_t conn;
    // Cache for GPU info
    struct amdgpu_gpu_info cached_info;
    int info_cached;
} rmapi_server_t;

void* handle_client(void* arg) {
    rmapi_server_t* server = (rmapi_server_t*)arg;
    ipc_message_t msg;

    while (ipc_recv_message(&server->conn, &msg) > 0) {
        switch (msg.type) {
            case 1: {  // IPC_ALLOC_MEMORY
                size_t size = *(size_t*)msg.data;
                struct amdgpu_buffer buf;
                amdgpu_buffer_alloc_hal(NULL, size, &buf);  // OBJGPU stub
                uint64_t addr = (uint64_t)buf.cpu_addr;
                ipc_send_message(&server->conn, &(ipc_message_t){2, msg.id, sizeof(addr), &addr});
                break;
            }
            case 3: {  // IPC_GET_GPU_INFO
                if (!server->info_cached) {
                    amdgpu_gpu_get_info_hal(NULL, &server->cached_info);
                    server->info_cached = 1;
                }
                ipc_send_message(&server->conn, &(ipc_message_t){4, msg.id, sizeof(server->cached_info), &server->cached_info});
                break;
            }
            // Add more cases
        }
    }
    return NULL;
}

int main() {
    rmapi_server_t server = {0};
    if (ipc_server_init(SOCKET_PATH, &server.conn) < 0) {
        perror("IPC init failed");
        return 1;
    }

    printf("RMAPI Server started, listening on %s\n", SOCKET_PATH);
    fflush(stdout);

    while (1) {
        struct sockaddr_un client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server.conn.sock_fd, (struct sockaddr*)&client_addr, &client_len);
        printf("Accepted client fd=%d\n", client_fd);
        fflush(stdout);
        if (client_fd >= 0) {
            ipc_connection_t client_conn = { .sock_fd = client_fd, .shm_addr = server.conn.shm_addr, .shm_size = server.conn.shm_size };
            rmapi_server_t* client_server = malloc(sizeof(rmapi_server_t));
            *client_server = server;
            client_server->conn = client_conn;
            pthread_t thread;
            pthread_create(&thread, NULL, handle_client, client_server);
            pthread_detach(thread);
        }
    }

    ipc_close(&server.conn);
    return 0;
}
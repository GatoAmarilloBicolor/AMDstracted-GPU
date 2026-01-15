#include "../common/ipc_lib.h"
#include "hal.h"
#include "rmapi.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

// RMAPI Server: Handles IPC requests for GPU operations
// Optimized with threading and caching
// Developed by: Haiku Imposible Team

#define SOCKET_PATH "/tmp/amdgpu_rmapi.sock"

typedef struct {
  ipc_connection_t conn;
  // Cache for GPU info
  struct amdgpu_gpu_info cached_info;
  int info_cached;
} rmapi_server_t;

void *handle_client(void *arg) {
  rmapi_server_t *server = (rmapi_server_t *)arg;
  ipc_message_t msg;

  while (ipc_recv_message(&server->conn, &msg) > 0) {
    switch (msg.type) {
    case 1: { // IPC_ALLOC_MEMORY
      size_t size = *(size_t *)msg.data;
      uint64_t addr;
      int ret = rmapi_alloc_memory(NULL, size, &addr);
      ipc_send_message(&server->conn,
                       &(ipc_message_t){2, msg.id, sizeof(addr), &addr});
      if (msg.data)
        free(msg.data);
      break;
    }
    case 3: { // IPC_GET_GPU_INFO
      struct amdgpu_gpu_info info;
      rmapi_get_gpu_info(NULL, &info);
      ipc_send_message(&server->conn,
                       &(ipc_message_t){4, msg.id, sizeof(info), &info});
      if (msg.data)
        free(msg.data);
      break;
    }
    case 5: { // IPC_FREE_MEMORY
      uint64_t addr = *(uint64_t *)msg.data;
      // In real RM, search buffer by addr
      // For now, stub success
      int success = 0;
      ipc_send_message(&server->conn,
                       &(ipc_message_t){6, msg.id, sizeof(success), &success});
      if (msg.data)
        free(msg.data);
      break;
    }
    case 7: { // IPC_SUBMIT_COMMAND
      // Data contains command stream (stub)
      struct amdgpu_command_buffer cb = {msg.data, msg.data_size};
      int ret = rmapi_submit_command(NULL, &cb);
      ipc_send_message(&server->conn,
                       &(ipc_message_t){8, msg.id, sizeof(ret), &ret});
      if (msg.data)
        free(msg.data);
      break;
    }
    }
  }
  ipc_close(&server->conn);
  free(server);
  return NULL;
}

int main() {
  rmapi_server_t server = {0};
  rmapi_init(); // Initialize global GPU and RMAPI

  if (ipc_server_init(SOCKET_PATH, &server.conn) < 0) {
    perror("IPC init failed");
    return 1;
  }

  printf("RMAPI Server started, listening on %s\n", SOCKET_PATH);
  fflush(stdout);

  while (1) {
    struct sockaddr_un client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server.conn.sock_fd, (struct sockaddr *)&client_addr,
                           &client_len);
    if (client_fd >= 0) {
      printf("Accepted client fd=%d\n", client_fd);
      fflush(stdout);
      rmapi_server_t *client_server = malloc(sizeof(rmapi_server_t));
      memset(client_server, 0, sizeof(rmapi_server_t));
      client_server->conn.sock_fd = client_fd;
      pthread_t thread;
      pthread_create(&thread, NULL, handle_client, client_server);
      pthread_detach(thread);
    }
  }

  ipc_close(&server.conn);
  return 0;
}
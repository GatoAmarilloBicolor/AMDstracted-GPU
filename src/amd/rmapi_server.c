#include "../../kernel-amd/os-primitives/os_primitives.h"
#include "../common/ipc_lib.h"
#include "../common/ipc_protocol.h"
#include "hal.h"
#include "rmapi.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

// Extern global GPU from rmapi.c
extern struct OBJGPU *global_gpu;

/*
 * Yo! This is the RMAPI Server - The "Brain" of our driver.
 * It listens for messages from your apps and tells the GPU what to do.
 * Think of it as the DJ in a club, taking requests and making the party happen.
 * Developed with pride by: Haiku Imposible Team (HIT)
 */

// Safe Shutdown: If the server crashes or someone stops it, we clean up!
void safe_shutdown(int sig) {
  printf(
      "\n[ALERT] Signal %d received! Cleaning up GPU city before leaving...\n",
      sig);
  rmapi_fini();
  unlink(HIT_SOCKET_PATH);
  exit(sig == SIGINT ? 0 : 1);
}

typedef struct {
  ipc_connection_t conn; // The "phone line" to the client
  // Saving some info here so we don't have to ask the GPU every time
  struct amdgpu_gpu_info cached_info;
  int info_cached;
} rmapi_server_t;

// This function handles a single client (an app).
// Since it uses threads, multiple apps can talk to the DJ at once!
void *handle_client(void *arg) {
  rmapi_server_t *server = (rmapi_server_t *)arg;
  ipc_message_t msg;

  // Keep listening as long as the app is talking
  while (ipc_recv_message(&server->conn, &msg) > 0) {
    switch (msg.type) {
    case IPC_REQ_ALLOC_MEMORY: { // REQUEST: I need GPU memory!
      size_t size = *(size_t *)msg.data;
      uint64_t addr;
      int ret =
          rmapi_alloc_memory(NULL, size, &addr); // Asking the HAL for space

      // Sending the address back to the app
      ipc_send_message(
          &server->conn,
          &(ipc_message_t){IPC_REP_ALLOC_MEMORY, msg.id, sizeof(addr), &addr});
      break;
    }
    case IPC_REQ_GET_GPU_INFO: { // REQUEST: Who is the GPU?
      struct amdgpu_gpu_info info;
      rmapi_get_gpu_info(NULL, &info);

      // Sending the GPU name and specs back!
      ipc_send_message(
          &server->conn,
          &(ipc_message_t){IPC_REP_GET_GPU_INFO, msg.id, sizeof(info), &info});
      break;
    }
    case IPC_REQ_FREE_MEMORY: { // REQUEST: I'm done with this memory
      uint64_t addr = *(uint64_t *)msg.data;
      int success = 0;
      ipc_send_message(&server->conn,
                       &(ipc_message_t){IPC_REP_FREE_MEMORY, msg.id,
                                        sizeof(success), &success});
      break;
    }
    case IPC_REQ_SUBMIT_COMMAND: { // REQUEST: Draw this!
      struct amdgpu_command_buffer cb = {msg.data, msg.data_size};
      int ret = rmapi_submit_command(NULL, &cb);

      // Tell the app if it worked
      ipc_send_message(
          &server->conn,
          &(ipc_message_t){IPC_REP_SUBMIT_COMMAND, msg.id, sizeof(ret), &ret});
      break;
    }
    case IPC_REQ_VK_CREATE_INSTANCE: {
      os_prim_log("RMAPI Server: VK_CREATE_INSTANCE received\n");
      void *instance = (void *)0xCAFEBABE; // Dummy handle
      os_prim_log("RMAPI Server: Returning instance handle %p\n", instance);
      ipc_send_message(&server->conn,
                       &(ipc_message_t){IPC_REP_VK_CREATE_INSTANCE, msg.id,
                                        sizeof(instance), &instance});
      break;
    }
    case IPC_REQ_VK_ENUMERATE_PHYSICAL_DEVICES: {
      os_prim_log("RMAPI Server: VK_ENUMERATE_PHYSICAL_DEVICES received\n");
      // Pack count + device list in response
      struct {
        uint32_t count;
        void *device;
      } response = {1, (void *)global_gpu};
      os_prim_log("RMAPI Server: Returning %u device(s)\n", response.count);
      ipc_send_message(&server->conn,
                       &(ipc_message_t){IPC_REP_VK_ENUMERATE_PHYSICAL_DEVICES,
                                        msg.id, sizeof(response), &response});
      break;
    }
    case IPC_REQ_VK_CREATE_DEVICE: {
      os_prim_log("RMAPI Server: VK_CREATE_DEVICE received\n");
      // Parse packed arguments
      struct {
        void *phys_dev;
        void *create_info;
      } *args = msg.data;
      void *device = (void *)0xDEADBEEF; // Dummy
      os_prim_log("RMAPI Server: Returning device handle %p\n", device);
      ipc_send_message(&server->conn,
                       &(ipc_message_t){IPC_REP_VK_CREATE_DEVICE, msg.id,
                                        sizeof(device), &device});
      break;
    }
    case IPC_REQ_VK_ALLOC_MEMORY: {
      os_prim_log("RMAPI Server: VK_ALLOC_MEMORY received\n");
      struct {
        void *device;
        void *alloc_info;
      } *args = msg.data;
      // TODO: Call real rmapi_alloc_memory
      void *memory = (void *)0xBEEFBEEF; // Dummy
      os_prim_log("RMAPI Server: Returning memory handle %p\n", memory);
      ipc_send_message(&server->conn,
                       &(ipc_message_t){IPC_REP_VK_ALLOC_MEMORY, msg.id,
                                        sizeof(memory), &memory});
      break;
    }
    case IPC_REQ_VK_FREE_MEMORY: {
      os_prim_log("RMAPI Server: VK_FREE_MEMORY received\n");
      struct {
        void *device;
        void *memory;
      } *args = msg.data;
      int ret = 0; // Success
      ipc_send_message(
          &server->conn,
          &(ipc_message_t){IPC_REP_VK_FREE_MEMORY, msg.id, sizeof(ret), &ret});
      break;
    }
    case IPC_REQ_VK_CREATE_COMMAND_POOL: {
      os_prim_log("RMAPI Server: VK_CREATE_COMMAND_POOL received\n");
      struct {
        void *device;
        void *create_info;
      } *args = msg.data;
      void *pool = (void *)0xFACEBEEF; // Dummy
      os_prim_log("RMAPI Server: Returning pool handle %p\n", pool);
      ipc_send_message(&server->conn,
                       &(ipc_message_t){IPC_REP_VK_CREATE_COMMAND_POOL, msg.id,
                                        sizeof(pool), &pool});
      break;
    }
    case IPC_REQ_VK_SUBMIT_QUEUE: {
      os_prim_log("RMAPI Server: VK_SUBMIT_QUEUE received\n");
      struct {
        void *queue;
        uint32_t count;
        void *submits;
        void *fence;
      } *args = msg.data;
      int ret = 0; // Success
      ipc_send_message(
          &server->conn,
          &(ipc_message_t){IPC_REP_VK_SUBMIT_QUEUE, msg.id, sizeof(ret), &ret});
      break;
    }
    }

    // --- Critical Fix: Free the message data after handling it ---
    if (msg.data) {
      free(msg.data);
      msg.data = NULL;
    }
  }

  // App disconnected or crashed. The DJ hangs up.
  ipc_close(&server->conn);
  free(server);
  return NULL;
}

#include <unistd.h>

int main() {
  // --- Safety First! ---
  // Catching crashes and interrupts to prevent hardware leftovers
  signal(SIGINT, safe_shutdown);
  signal(SIGSEGV, safe_shutdown);
  signal(SIGTERM, safe_shutdown);

  rmapi_server_t server = {0};

  // Starting the brain and setting up the specialists
  rmapi_init();

  // Building the "subway station" where apps can connect
  if (ipc_server_init(HIT_SOCKET_PATH, &server.conn) < 0) {
    perror("Aw man, IPC init failed! Maybe the socket is already in use?");
    return 1;
  }

  printf("Yo! RMAPI Server is live on %s. Ready to work!\n", HIT_SOCKET_PATH);
  fflush(stdout);

  // Loop forever, waiting for new apps to connect
  while (1) {
    struct sockaddr_un client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server.conn.sock_fd, (struct sockaddr *)&client_addr,
                           &client_len);
    if (client_fd >= 0) {
      printf("A new app just connected! (Client fd=%d)\n", client_fd);
      fflush(stdout);

      // Give the app their own little server space
      rmapi_server_t *client_server = malloc(sizeof(rmapi_server_t));
      memset(client_server, 0, sizeof(rmapi_server_t));
      client_server->conn.sock_fd = client_fd;

      // Start a new thread so we don't block other apps!
      pthread_t thread;
      pthread_create(&thread, NULL, handle_client, client_server);
      pthread_detach(thread);
    }
  }

  ipc_close(&server.conn);
  return 0;
}
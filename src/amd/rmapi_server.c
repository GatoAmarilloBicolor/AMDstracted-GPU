#include "../common/ipc_lib.h"
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

/*
 * Yo! This is the RMAPI Server - The "Brain" of our driver.
 * It listens for messages from your apps and tells the GPU what to do.
 * Think of it as the DJ in a club, taking requests and making the party happen.
 * Developed with pride by: Haiku Imposible Team (HIT)
 */

#define SOCKET_PATH "/tmp/amdgpu_rmapi.sock"

// Safe Shutdown: If the server crashes or someone stops it, we clean up!
void safe_shutdown(int sig) {
  printf(
      "\n[ALERT] Signal %d received! Cleaning up GPU city before leaving...\n",
      sig);
  rmapi_fini();
  unlink(SOCKET_PATH);
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
    case 1: { // REQUEST: I need GPU memory! (IPC_ALLOC_MEMORY)
      size_t size = *(size_t *)msg.data;
      uint64_t addr;
      int ret =
          rmapi_alloc_memory(NULL, size, &addr); // Asking the HAL for space

      // Sending the address back to the app
      ipc_send_message(&server->conn,
                       &(ipc_message_t){2, msg.id, sizeof(addr), &addr});
      if (msg.data)
        free(msg.data);
      break;
    }
    case 3: { // REQUEST: Who is the GPU? (IPC_GET_GPU_INFO)
      struct amdgpu_gpu_info info;
      rmapi_get_gpu_info(NULL, &info);

      // Sending the GPU name and specs back!
      ipc_send_message(&server->conn,
                       &(ipc_message_t){4, msg.id, sizeof(info), &info});
      if (msg.data)
        free(msg.data);
      break;
    }
    case 5: { // REQUEST: I'm done with this memory (IPC_FREE_MEMORY)
      uint64_t addr = *(uint64_t *)msg.data;
      // In a real driver, we'd search and destroy.
      // For now, we just give a thumbs up!
      int success = 0;
      ipc_send_message(&server->conn,
                       &(ipc_message_t){6, msg.id, sizeof(success), &success});
      if (msg.data)
        free(msg.data);
      break;
    }
    case 7: { // REQUEST: Draw this! (IPC_SUBMIT_COMMAND)
      // We wrap the list of commands and send them to the "Artist" (GFX Block)
      struct amdgpu_command_buffer cb = {msg.data, msg.data_size};
      int ret = rmapi_submit_command(NULL, &cb);

      // Tell the app if it worked
      ipc_send_message(&server->conn,
                       &(ipc_message_t){8, msg.id, sizeof(ret), &ret});
      if (msg.data)
        free(msg.data);
      break;
    }
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
  if (ipc_server_init(SOCKET_PATH, &server.conn) < 0) {
    perror("Aw man, IPC init failed! Maybe the socket is already in use?");
    return 1;
  }

  printf("Yo! RMAPI Server is live on %s. Ready to work!\n", SOCKET_PATH);
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
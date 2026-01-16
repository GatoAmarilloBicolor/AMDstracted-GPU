#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#include "ipc_lib.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

/*
 * Yo! This is the IPC Library (The Universal Subway System).
 */

static pthread_mutex_t ipc_mutex = PTHREAD_MUTEX_INITIALIZER;

#define HIT_SHM_SIZE (1024 * 1024) // 1MB fast-track
#define HIT_SHM_NAME "/hit_subway_shm"

// Setting up the "Subway Station" (Server side)
int ipc_server_init(const char *socket_path, ipc_connection_t *conn) {
  if (!socket_path || !conn)
    return -1;

  memset(conn, 0, sizeof(ipc_connection_t));

  // 1. Create the socket (The "phone line")
  conn->sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (conn->sock_fd < 0)
    return -1;

  // 2. Set the address
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
  unlink(socket_path);

  if (bind(conn->sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(conn->sock_fd);
    return -1;
  }

  if (listen(conn->sock_fd, 5) < 0) {
    close(conn->sock_fd);
    return -1;
  }

  // 3. Create the Fast-Path (Shared Memory)
  int shm_fd = shm_open(HIT_SHM_NAME, O_CREAT | O_RDWR, 0666);
  if (shm_fd >= 0) {
    ftruncate(shm_fd, HIT_SHM_SIZE);
    conn->shm_addr =
        mmap(NULL, HIT_SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    conn->shm_size = HIT_SHM_SIZE;
    close(shm_fd);
    printf("[LOG] IPC: Fast-Path subway line (SHM) is open!\n");
  }

  conn->epoll_fd = -1;
  return 0;
}

// Connecting to the "Subway Station" (Client side)
int ipc_client_connect(const char *socket_path, ipc_connection_t *conn) {
  if (!socket_path || !conn)
    return -1;

  memset(conn, 0, sizeof(ipc_connection_t));
  conn->sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (conn->sock_fd < 0)
    return -1;

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

  if (connect(conn->sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(conn->sock_fd);
    return -1;
  }

  // Map the Fast-Path
  int shm_fd = shm_open(HIT_SHM_NAME, O_RDWR, 0666);
  if (shm_fd >= 0) {
    conn->shm_addr =
        mmap(NULL, HIT_SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    conn->shm_size = HIT_SHM_SIZE;
    close(shm_fd);
  }

  return 0;
}

// Sending a message through the tunnel
int ipc_send_message(ipc_connection_t *conn, ipc_message_t *msg) {
  if (!conn || !msg)
    return -1;

  pthread_mutex_lock(&ipc_mutex);

  // If data is already in SHM (pointer in range), don't send it via socket!
  void *original_data = msg->data;
  if (conn->shm_addr && original_data >= conn->shm_addr &&
      (uint8_t *)original_data < (uint8_t *)conn->shm_addr + conn->shm_size) {
    msg->data = NULL; // Flag for remote: "Data is in SHM, check your map!"
  }

  ssize_t sent = send(conn->sock_fd, msg, sizeof(ipc_message_t), 0);
  msg->data = original_data; // Restore

  if (sent != sizeof(ipc_message_t)) {
    pthread_mutex_unlock(&ipc_mutex);
    return -1;
  }

  // Only copy if NOT using the SHM fast-path
  if (msg->data && msg->data_size > 0 && msg->data != NULL &&
      !(conn->shm_addr && original_data >= conn->shm_addr &&
        (uint8_t *)original_data <
            (uint8_t *)conn->shm_addr + conn->shm_size)) {
    sent = send(conn->sock_fd, msg->data, msg->data_size, 0);
    if (sent != (ssize_t)msg->data_size) {
      pthread_mutex_unlock(&ipc_mutex);
      return -1;
    }
  }

  pthread_mutex_unlock(&ipc_mutex);
  return 0;
}

// Receiving a message from the tunnel
int ipc_recv_message(ipc_connection_t *conn, ipc_message_t *msg) {
  if (!conn || !msg)
    return -1;

  ssize_t recvd = recv(conn->sock_fd, msg, sizeof(ipc_message_t), 0);
  if (recvd <= 0)
    return recvd;

  if (recvd != sizeof(ipc_message_t))
    return -1;

  // Check if this is a Fast-Path message
  if (msg->data_size > 0 && msg->data == NULL && conn->shm_addr) {
    msg->data = conn->shm_addr; // Data is waiting in the SHM buffer!
    return 1;
  }

  if (msg->data_size > 0) {
    msg->data = malloc(msg->data_size);
    if (!msg->data)
      return -1;

    recvd = recv(conn->sock_fd, msg->data, msg->data_size, MSG_WAITALL);
    if (recvd != (ssize_t)msg->data_size) {
      free(msg->data);
      return -1;
    }
  }

  return 1;
}

// Closing the connection
void ipc_close(ipc_connection_t *conn) {
  if (!conn)
    return;

  if (conn->shm_addr) {
    munmap(conn->shm_addr, conn->shm_size);
    shm_unlink(HIT_SHM_NAME);
  }

  if (conn->sock_fd >= 0)
    close(conn->sock_fd);
  memset(conn, 0, sizeof(ipc_connection_t));
}
#include "ipc_lib.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

/*
 * Yo! This is the IPC Library (The subway system).
 * It manages how the apps talk to the RMAPI Server.
 * We use Unix Sockets because they are reliable and fast.
 *
 * Developed by: Haiku Imposible Team (HIT)
 */

// A simple lock so messages don't get mixed up if multiple threads are talking
static pthread_mutex_t ipc_mutex = PTHREAD_MUTEX_INITIALIZER;

// Setting up the "Subway Station" (Server side)
int ipc_server_init(const char *socket_path, ipc_connection_t *conn) {
  if (!socket_path || !conn)
    return -1;

  memset(conn, 0, sizeof(ipc_connection_t));

  // 1. Create the "phone line" (Socket)
  conn->sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (conn->sock_fd < 0)
    return -1;

  // 2. Set the address (A file on your disk!)
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
  unlink(socket_path); // Delete the old station if it exists

  // 3. Connect the line to the address
  if (bind(conn->sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(conn->sock_fd);
    return -1;
  }

  // 4. Start listening for incoming "phone calls"
  if (listen(conn->sock_fd, 5) < 0) {
    close(conn->sock_fd);
    return -1;
  }

  // 5. Setting up "Caller ID" (epoll) to see who is calling!
  conn->epoll_fd = epoll_create1(0);
  if (conn->epoll_fd >= 0) {
    struct epoll_event ev = {.events = EPOLLIN, .data.fd = conn->sock_fd};
    epoll_ctl(conn->epoll_fd, EPOLL_CTL_ADD, conn->sock_fd, &ev);
  }

  return 0;
}

// Connecting to a "Subway Station" (Client side)
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

  // Trying to reach the city hall!
  if (connect(conn->sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(conn->sock_fd);
    return -1;
  }

  return 0;
}

// Sending a message through the tunnel
int ipc_send_message(ipc_connection_t *conn, ipc_message_t *msg) {
  if (!conn || !msg)
    return -1;

  pthread_mutex_lock(&ipc_mutex); // Grab the lock!

  // 1. Send the "envelope" with the metadata
  ssize_t sent = send(conn->sock_fd, msg, sizeof(ipc_message_t), 0);
  if (sent != sizeof(ipc_message_t)) {
    pthread_mutex_unlock(&ipc_mutex);
    return -1;
  }

  // 2. Send the actual "letter" (the data) if there is any
  if (msg->data && msg->data_size > 0) {
    sent = send(conn->sock_fd, msg->data, msg->data_size, 0);
    if (sent != (ssize_t)msg->data_size) {
      pthread_mutex_unlock(&ipc_mutex);
      return -1;
    }
  }

  pthread_mutex_unlock(&ipc_mutex); // All done!
  return 0;
}

// Receiving a message from the tunnel
int ipc_recv_message(ipc_connection_t *conn, ipc_message_t *msg) {
  if (!conn || !msg)
    return -1;

  // 1. Receive the "envelope"
  ssize_t recvd = recv(conn->sock_fd, msg, sizeof(ipc_message_t), 0);
  if (recvd <= 0)
    return recvd; // They hung up or something went wrong

  if (recvd != sizeof(ipc_message_t))
    return -1;

  // 2. If there is a "letter" inside, we need space to read it
  if (msg->data_size > 0) {
    msg->data = malloc(msg->data_size);
    if (!msg->data)
      return -1;

    // Read the whole letter
    recvd = recv(conn->sock_fd, msg->data, msg->data_size, MSG_WAITALL);
    if (recvd != (ssize_t)msg->data_size) {
      free(msg->data);
      return -1;
    }
  } else {
    msg->data = NULL;
  }

  return 1;
}

// Hanging up the phone
void ipc_close(ipc_connection_t *conn) {
  if (!conn)
    return;
  if (conn->sock_fd >= 0)
    close(conn->sock_fd);
  if (conn->epoll_fd >= 0)
    close(conn->epoll_fd);
  memset(conn, 0, sizeof(ipc_connection_t));
}
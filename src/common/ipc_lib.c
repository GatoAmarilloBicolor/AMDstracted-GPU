#include "ipc_lib.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

/*
 * Yo! This is the IPC Library (The Universal Subway System).
 *
 * We removed all the Linux-only "epoll" stuff to make sure this works on
 * ANY system that calls itself POSIX (Haiku, BSD, even exotic ones!).
 *
 * It uses standard Unix Domain Sockets which are the gold standard for
 * fast and safe local communication.
 *
 * Developed by: Haiku Imposible Team (HIT)
 */

static pthread_mutex_t ipc_mutex = PTHREAD_MUTEX_INITIALIZER;

// Setting up the "Subway Station" (Server side)
int ipc_server_init(const char *socket_path, ipc_connection_t *conn) {
  if (!socket_path || !conn)
    return -1;

  memset(conn, 0, sizeof(ipc_connection_t));

  // 1. Create the socket (The "phone line")
  conn->sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (conn->sock_fd < 0)
    return -1;

  // 2. Set the address (A special file on your disk!)
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
  unlink(socket_path); // Clear out any old station leftover

  // 3. Bind the line to the address
  if (bind(conn->sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(conn->sock_fd);
    return -1;
  }

  // 4. Start listening for apps trying to connect
  if (listen(conn->sock_fd, 5) < 0) {
    close(conn->sock_fd);
    return -1;
  }

  // We set epoll_fd to -1 because we chose to keep it super
  // simple and portable for all OSes!
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

  // Trying to reach the Server!
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

  pthread_mutex_lock(&ipc_mutex);

  // 1. Send the "envelope" (Metadata)
  ssize_t sent = send(conn->sock_fd, msg, sizeof(ipc_message_t), 0);
  if (sent != sizeof(ipc_message_t)) {
    pthread_mutex_unlock(&ipc_mutex);
    return -1;
  }

  // 2. Send the "letter" (The actual data) if any
  if (msg->data && msg->data_size > 0) {
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

  // 1. Receive the "envelope"
  ssize_t recvd = recv(conn->sock_fd, msg, sizeof(ipc_message_t), 0);
  if (recvd <= 0)
    return recvd;

  if (recvd != sizeof(ipc_message_t))
    return -1;

  // 2. Read the "letter" inside
  if (msg->data_size > 0) {
    msg->data = malloc(msg->data_size);
    if (!msg->data)
      return -1;

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

// Closing the connection
void ipc_close(ipc_connection_t *conn) {
  if (!conn)
    return;
  if (conn->sock_fd >= 0)
    close(conn->sock_fd);
  memset(conn, 0, sizeof(ipc_connection_t));
}
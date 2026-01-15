#include "ipc_lib.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static pthread_mutex_t ipc_mutex = PTHREAD_MUTEX_INITIALIZER;

int ipc_server_init(const char *socket_path, ipc_connection_t *conn) {
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
  unlink(socket_path);

  if (bind(conn->sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(conn->sock_fd);
    return -1;
  }

  if (listen(conn->sock_fd, 5) < 0) {
    close(conn->sock_fd);
    return -1;
  }

  conn->epoll_fd = epoll_create1(0);
  if (conn->epoll_fd >= 0) {
    struct epoll_event ev = {.events = EPOLLIN, .data.fd = conn->sock_fd};
    epoll_ctl(conn->epoll_fd, EPOLL_CTL_ADD, conn->sock_fd, &ev);
  }

  return 0;
}

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

  return 0;
}

int ipc_send_message(ipc_connection_t *conn, ipc_message_t *msg) {
  if (!conn || !msg)
    return -1;
  pthread_mutex_lock(&ipc_mutex);

  // Send header/metadata
  ssize_t sent = send(conn->sock_fd, msg, sizeof(ipc_message_t), 0);
  if (sent != sizeof(ipc_message_t)) {
    pthread_mutex_unlock(&ipc_mutex);
    return -1;
  }

  // Send payload if any
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

int ipc_recv_message(ipc_connection_t *conn, ipc_message_t *msg) {
  if (!conn || !msg)
    return -1;

  // Receive header
  ssize_t recvd = recv(conn->sock_fd, msg, sizeof(ipc_message_t), 0);
  if (recvd <= 0)
    return recvd;
  if (recvd != sizeof(ipc_message_t))
    return -1;

  // Receive payload if any
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

void ipc_close(ipc_connection_t *conn) {
  if (!conn)
    return;
  if (conn->sock_fd >= 0)
    close(conn->sock_fd);
  if (conn->epoll_fd >= 0)
    close(conn->epoll_fd);
  memset(conn, 0, sizeof(ipc_connection_t));
}
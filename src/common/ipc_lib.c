// Enhanced IPC Library with isolation (locking, async)
// Recycled from NVIDIA-Haiku IPC patterns

#include <pthread.h>
#include <fcntl.h>
#include <sys/epoll.h>

// Add mutex for isolation
static pthread_mutex_t ipc_mutex = PTHREAD_MUTEX_INITIALIZER;

int ipc_server_init(const char* socket_path, ipc_connection_t* conn) {
    // Existing code...
    // Add: epoll for async
    conn->epoll_fd = epoll_create1(0);
    struct epoll_event ev = {.events = EPOLLIN, .data.fd = conn->sock_fd};
    epoll_ctl(conn->epoll_fd, EPOLL_CTL_ADD, conn->sock_fd, &ev);
    return 0;
}

int ipc_send_message(ipc_connection_t* conn, ipc_message_t* msg) {
    pthread_mutex_lock(&ipc_mutex);  // Isolation
    int ret = send(conn->sock_fd, msg, sizeof(ipc_message_t), 0);
    pthread_mutex_unlock(&ipc_mutex);
    return ret;
}
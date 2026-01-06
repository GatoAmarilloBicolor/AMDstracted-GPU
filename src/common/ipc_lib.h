#ifndef IPC_LIB_H
#define IPC_LIB_H

#include <stdint.h>
#include <stddef.h>

// IPC Library for optimized userland server communication
// Zero-copy, async, portable across POSIX OSes

typedef struct {
    int sock_fd;  // Socket for messages
    void* shm_addr;  // Shared memory for zero-copy
    size_t shm_size;
    int epoll_fd;  // For async (optional)
} ipc_connection_t;

// Messages
typedef struct {
    uint32_t type;  // e.g., IPC_ALLOC_BUFFER
    uint32_t id;    // Request ID
    size_t data_size;
    void* data;     // Zero-copy via shm
} ipc_message_t;

// Init IPC server
int ipc_server_init(const char* socket_path, ipc_connection_t* conn);

// Init IPC client
int ipc_client_connect(const char* socket_path, ipc_connection_t* conn);

// Send message (async)
int ipc_send_message(ipc_connection_t* conn, ipc_message_t* msg);

// Receive message (async)
int ipc_recv_message(ipc_connection_t* conn, ipc_message_t* msg);

// Cleanup
void ipc_close(ipc_connection_t* conn);

#endif
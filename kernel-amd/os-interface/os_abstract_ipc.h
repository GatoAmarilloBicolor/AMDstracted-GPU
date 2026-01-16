/*
 * OS-Agnostic IPC Layer
 * 
 * Abstracts IPC mechanism to work across Linux, Haiku, FreeBSD, etc.
 * Supports: Unix sockets, message queues, pipes, named pipes
 * 
 * Developed by: Haiku Imposible Team (HIT)
 */

#ifndef OS_ABSTRACT_IPC_H
#define OS_ABSTRACT_IPC_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================================
 * IPC TRANSPORT TYPES
 * ============================================================================ */

typedef enum {
    OS_IPC_UNIX_SOCKET,    // POSIX: Linux, Haiku, FreeBSD (preferred)
    OS_IPC_NAMED_PIPE,     // FIFO: POSIX compatible
    OS_IPC_MESSAGE_QUEUE,  // POSIX mqueue: Linux, Haiku, FreeBSD
    OS_IPC_SHARED_MEMORY,  // POSIX shm: Lowest overhead
} os_ipc_type_t;

typedef struct {
    os_ipc_type_t type;
    int fd;                // File descriptor or handle
    void *private_data;    // OS-specific data
    char endpoint[256];    // Socket path, pipe name, queue name, etc.
} os_ipc_connection_t;

typedef struct {
    void *data;
    size_t size;
    uint32_t timeout_ms;  // 0 = blocking
} os_ipc_message_t;

/* ============================================================================
 * IPC SERVER (receives connections)
 * ============================================================================ */

/**
 * Create an IPC server endpoint (listener)
 * 
 * @param type         IPC transport type to use
 * @param endpoint     Endpoint name (socket path, queue name, etc.)
 * @param max_backlog  Maximum pending connections
 * @return Server handle, or NULL on error
 */
typedef void* os_ipc_server_t;
os_ipc_server_t os_ipc_server_create(os_ipc_type_t type, const char *endpoint,
                                    int max_backlog);

/**
 * Accept a connection from a client
 */
os_ipc_connection_t* os_ipc_server_accept(os_ipc_server_t server,
                                         uint32_t timeout_ms);

/**
 * Send reply to client
 */
int os_ipc_send(os_ipc_connection_t *conn, const void *data, size_t size);

/**
 * Receive message from client
 */
int os_ipc_recv(os_ipc_connection_t *conn, void *buf, size_t buf_size,
               uint32_t timeout_ms);

/**
 * Close connection
 */
void os_ipc_disconnect(os_ipc_connection_t *conn);

/**
 * Destroy server
 */
void os_ipc_server_destroy(os_ipc_server_t server);

/* ============================================================================
 * IPC CLIENT (connects to server)
 * ============================================================================ */

/**
 * Connect to an IPC server
 */
os_ipc_connection_t* os_ipc_client_connect(os_ipc_type_t type,
                                          const char *endpoint,
                                          uint32_t timeout_ms);

/**
 * Send request to server
 */
int os_ipc_client_send(os_ipc_connection_t *conn, const void *data, size_t size);

/**
 * Receive response from server
 */
int os_ipc_client_recv(os_ipc_connection_t *conn, void *buf, size_t buf_size,
                      uint32_t timeout_ms);

/* ============================================================================
 * SHARED MEMORY (for bulk data transfer)
 * ============================================================================ */

typedef struct {
    void *addr;        // Virtual address
    size_t size;
    const char *name;  // Shared memory identifier
    int fd;            // File descriptor
} os_ipc_shm_t;

/**
 * Create or open shared memory segment
 */
os_ipc_shm_t* os_ipc_shm_create(const char *name, size_t size);

/**
 * Get shared memory segment (for clients)
 */
os_ipc_shm_t* os_ipc_shm_open(const char *name);

/**
 * Close shared memory
 */
void os_ipc_shm_close(os_ipc_shm_t *shm);

#endif // OS_ABSTRACT_IPC_H

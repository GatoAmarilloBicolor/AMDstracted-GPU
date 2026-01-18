/*
 * OS-Agnostic IPC Layer - Implementation
 * 
 * Currently implements POSIX Unix sockets (works everywhere)
 * Can be extended with platform-specific optimizations
 * 
 * Developed by: Haiku Imposible Team (HIT)
 */

#include "os_abstract_ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>

/* ============================================================================
 * HELPER: Logging
 * ============================================================================ */

static void os_ipc_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[IPC] ");
    vfprintf(stderr, fmt, args);
    va_end(args);
}

#include <stdarg.h>

/* ============================================================================
 * IPC SERVER - Unix Socket Implementation
 * ============================================================================ */

typedef struct {
    int socket_fd;
    struct sockaddr_un addr;
    char endpoint[256];
    int backlog;
} os_ipc_server_unix_t;

os_ipc_server_t os_ipc_server_create(os_ipc_type_t type, const char *endpoint,
                                     int max_backlog) {
    if (!endpoint) return NULL;
    
    // Only Unix sockets for now
    if (type != OS_IPC_UNIX_SOCKET) {
        os_ipc_log("ERROR: Only Unix sockets supported\n");
        return NULL;
    }
    
    os_ipc_server_unix_t *server = malloc(sizeof(*server));
    if (!server) return NULL;
    
    // Create Unix socket
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        os_ipc_log("ERROR: socket() failed: %s\n", strerror(errno));
        free(server);
        return NULL;
    }
    
    // Prepare address
    memset(&server->addr, 0, sizeof(server->addr));
    server->addr.sun_family = AF_UNIX;
    strncpy(server->addr.sun_path, endpoint, sizeof(server->addr.sun_path) - 1);
    
    // Remove existing socket
    unlink(endpoint);
    
    // Bind
    if (bind(fd, (struct sockaddr *)&server->addr, sizeof(server->addr)) < 0) {
        os_ipc_log("ERROR: bind(%s) failed: %s\n", endpoint, strerror(errno));
        close(fd);
        free(server);
        return NULL;
    }
    
    // Listen
    if (listen(fd, max_backlog) < 0) {
        os_ipc_log("ERROR: listen() failed: %s\n", strerror(errno));
        close(fd);
        free(server);
        return NULL;
    }
    
    server->socket_fd = fd;
    server->backlog = max_backlog;
    strncpy(server->endpoint, endpoint, sizeof(server->endpoint) - 1);
    
    os_ipc_log("Server created on %s (fd=%d)\n", endpoint, fd);
    
    return (os_ipc_server_t)server;
}

os_ipc_connection_t* os_ipc_server_accept(os_ipc_server_t server,
                                          uint32_t timeout_ms) {
    os_ipc_server_unix_t *srv = (os_ipc_server_unix_t *)server;
    if (!srv) return NULL;
    
    // Accept connection
    struct sockaddr_un client_addr;
    socklen_t addrlen = sizeof(client_addr);
    
    int client_fd = accept(srv->socket_fd, (struct sockaddr *)&client_addr,
                          &addrlen);
    if (client_fd < 0) {
        os_ipc_log("ERROR: accept() failed: %s\n", strerror(errno));
        return NULL;
    }
    
    // Create connection object
    os_ipc_connection_t *conn = malloc(sizeof(*conn));
    if (!conn) {
        close(client_fd);
        return NULL;
    }
    
    memset(conn, 0, sizeof(*conn));
    conn->type = OS_IPC_UNIX_SOCKET;
    conn->fd = client_fd;
    strncpy(conn->endpoint, srv->endpoint, sizeof(conn->endpoint) - 1);
    
    os_ipc_log("Accepted connection on fd=%d\n", client_fd);
    
    return conn;
}

int os_ipc_send(os_ipc_connection_t *conn, const void *data, size_t size) {
    if (!conn || !data) return -1;
    
    ssize_t sent = send(conn->fd, data, size, 0);
    if (sent < 0) {
        os_ipc_log("ERROR: send() failed: %s\n", strerror(errno));
        return -1;
    }
    
    return (int)sent;
}

int os_ipc_recv(os_ipc_connection_t *conn, void *buf, size_t buf_size,
               uint32_t timeout_ms) {
    if (!conn || !buf) return -1;
    
    // Set socket timeout if requested
    if (timeout_ms > 0) {
        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        setsockopt(conn->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }
    
    ssize_t received = recv(conn->fd, buf, buf_size, 0);
    if (received < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            os_ipc_log("ERROR: recv() failed: %s\n", strerror(errno));
        }
        return -1;
    }
    
    return (int)received;
}

void os_ipc_disconnect(os_ipc_connection_t *conn) {
    if (!conn) return;
    
    if (conn->fd >= 0) {
        close(conn->fd);
        conn->fd = -1;
    }
    
    free(conn);
}

void os_ipc_server_destroy(os_ipc_server_t server) {
    os_ipc_server_unix_t *srv = (os_ipc_server_unix_t *)server;
    if (!srv) return;
    
    if (srv->socket_fd >= 0) {
        close(srv->socket_fd);
    }
    
    unlink(srv->endpoint);
    free(srv);
}

/* ============================================================================
 * IPC CLIENT - Unix Socket Implementation
 * ============================================================================ */

os_ipc_connection_t* os_ipc_client_connect(os_ipc_type_t type,
                                          const char *endpoint,
                                          uint32_t timeout_ms) {
    if (!endpoint || type != OS_IPC_UNIX_SOCKET) {
        return NULL;
    }
    
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        os_ipc_log("ERROR: socket() failed: %s\n", strerror(errno));
        return NULL;
    }
    
    // Set timeout
    if (timeout_ms > 0) {
        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, endpoint, sizeof(addr.sun_path) - 1);
    
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        os_ipc_log("ERROR: connect(%s) failed: %s\n", endpoint, strerror(errno));
        close(fd);
        return NULL;
    }
    
    os_ipc_connection_t *conn = malloc(sizeof(*conn));
    if (!conn) {
        close(fd);
        return NULL;
    }
    
    memset(conn, 0, sizeof(*conn));
    conn->type = OS_IPC_UNIX_SOCKET;
    conn->fd = fd;
    strncpy(conn->endpoint, endpoint, sizeof(conn->endpoint) - 1);
    
    os_ipc_log("Client connected to %s (fd=%d)\n", endpoint, fd);
    
    return conn;
}

int os_ipc_client_send(os_ipc_connection_t *conn, const void *data, size_t size) {
    return os_ipc_send(conn, data, size);
}

int os_ipc_client_recv(os_ipc_connection_t *conn, void *buf, size_t buf_size,
                      uint32_t timeout_ms) {
    return os_ipc_recv(conn, buf, buf_size, timeout_ms);
}

/* ============================================================================
 * SHARED MEMORY - POSIX Implementation
 * ============================================================================ */

os_ipc_shm_t* os_ipc_shm_create(const char *name, size_t size) {
    if (!name || size == 0) return NULL;
    
    // Create shared memory object
    int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        os_ipc_log("ERROR: shm_open(%s) failed: %s\n", name, strerror(errno));
        return NULL;
    }
    
    // Resize to requested size
    if (ftruncate(fd, size) < 0) {
        os_ipc_log("ERROR: ftruncate() failed: %s\n", strerror(errno));
        close(fd);
        return NULL;
    }
    
    // Map into memory
    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        os_ipc_log("ERROR: mmap() failed: %s\n", strerror(errno));
        close(fd);
        return NULL;
    }
    
    os_ipc_shm_t *shm = malloc(sizeof(*shm));
    if (!shm) {
        munmap(addr, size);
        close(fd);
        return NULL;
    }
    
    shm->addr = addr;
    shm->size = size;
    shm->fd = fd;
    shm->name = strdup(name);
    
    os_ipc_log("Created shared memory %s (%zu bytes)\n", name, size);
    
    return shm;
}

os_ipc_shm_t* os_ipc_shm_open(const char *name) {
    if (!name) return NULL;
    
    int fd = shm_open(name, O_RDWR, 0666);
    if (fd < 0) {
        os_ipc_log("ERROR: shm_open(%s) failed: %s\n", name, strerror(errno));
        return NULL;
    }
    
    // Get size via fstat
    struct stat st;
    if (fstat(fd, &st) < 0) {
        os_ipc_log("ERROR: fstat() failed: %s\n", strerror(errno));
        close(fd);
        return NULL;
    }
    
    size_t size = st.st_size;
    
    // Map into memory
    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        os_ipc_log("ERROR: mmap() failed: %s\n", strerror(errno));
        close(fd);
        return NULL;
    }
    
    os_ipc_shm_t *shm = malloc(sizeof(*shm));
    if (!shm) {
        munmap(addr, size);
        close(fd);
        return NULL;
    }
    
    shm->addr = addr;
    shm->size = size;
    shm->fd = fd;
    shm->name = strdup(name);
    
    os_ipc_log("Opened shared memory %s (%zu bytes)\n", name, size);
    
    return shm;
}

void os_ipc_shm_close(os_ipc_shm_t *shm) {
    if (!shm) return;
    
    if (shm->addr) {
        munmap(shm->addr, shm->size);
    }
    
    if (shm->fd >= 0) {
        close(shm->fd);
    }
    
    if (shm->name) {
        free((void *)shm->name);
    }
    
    free(shm);
}

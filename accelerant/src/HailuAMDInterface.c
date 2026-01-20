/*
 * HailuAMDInterface.c
 * RMAPI Bridge Layer for Haiku Accelerant
 * 
 * Connects accelerant hooks to AMDGPU_Abstracted RMAPI server
 * Implements GPU operations via IPC and command submission
 * 
 * Copyright (c) 2024-2026 AMDGPU_Abstracted Project
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <kernel/OS.h>
#include <Errors.h>

/* Haiku status constants (may not be in older headers) */
#ifndef B_CONNECTION_REFUSED
#define B_CONNECTION_REFUSED ECONNREFUSED
#endif
#ifndef B_NOT_INITIALIZED
#define B_NOT_INITIALIZED ENXIO
#endif

/* ============================================================================
 * RMAPI Server Communication
 * ============================================================================ */

#define RMAPI_SERVER_PORT_NAME "amd_rmapi_server"
#define RMAPI_SOCKET_PATH "/var/run/amd_rmapi.sock"

typedef struct {
    port_id port;
    int socket_fd;
} rmapi_connection;

static rmapi_connection g_rmapi = {.port = -1, .socket_fd = -1};
static pthread_mutex_t g_rmapi_lock = PTHREAD_MUTEX_INITIALIZER;

/* ============================================================================
 * RMAPI Message Types
 * ============================================================================ */

typedef enum {
    RMAPI_CMD_INIT = 0x1001,
    RMAPI_CMD_SHUTDOWN = 0x1002,
    RMAPI_CMD_GET_DISPLAY_INFO = 0x2001,
    RMAPI_CMD_SET_DISPLAY_MODE = 0x2002,
    RMAPI_CMD_ALLOCATE_MEMORY = 0x3001,
    RMAPI_CMD_FREE_MEMORY = 0x3002,
    RMAPI_CMD_SUBMIT_COMMAND = 0x4001,
    RMAPI_CMD_WAIT_FENCE = 0x4002,
} rmapi_command_t;

typedef struct {
    rmapi_command_t cmd;
    uint32_t flags;
    uint64_t param1;
    uint64_t param2;
    uint64_t param3;
} rmapi_request;

typedef struct {
    status_t result;
    uint32_t flags;
    uint64_t result1;
    uint64_t result2;
} rmapi_response;

/* ============================================================================
 * Haiku Port-based IPC (Primary method)
 * ============================================================================ */

static status_t
rmapi_connect_haiku_port(void)
{
    pthread_mutex_lock(&g_rmapi_lock);
    
    /* Try to find existing server port */
    g_rmapi.port = find_port(RMAPI_SERVER_PORT_NAME);
    if (g_rmapi.port >= 0) {
        pthread_mutex_unlock(&g_rmapi_lock);
        return B_OK;
    }
    
    pthread_mutex_unlock(&g_rmapi_lock);
    return B_NAME_NOT_FOUND;
}

static status_t
rmapi_send_message_haiku(rmapi_request *req, rmapi_response *resp)
{
    if (g_rmapi.port < 0)
        return B_WOULD_BLOCK;
    
    /* Send request via port */
    ssize_t sent = write_port(g_rmapi.port, (int32_t)req->cmd, req, sizeof(*req));
    if (sent != sizeof(*req))
        return B_IO_ERROR;
    
    /* Read response */
    int32_t code;
    ssize_t received = read_port(g_rmapi.port, &code, resp, sizeof(*resp));
    if (received != sizeof(*resp))
        return B_IO_ERROR;
    
    return resp->result;
}

/* ============================================================================
 * Unix Socket IPC (Fallback for non-Haiku)
 * ============================================================================ */

static status_t
rmapi_connect_socket(void)
{
    struct sockaddr_un addr;
    
    g_rmapi.socket_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (g_rmapi.socket_fd < 0)
        return B_IO_ERROR;
    
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, RMAPI_SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    if (connect(g_rmapi.socket_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(g_rmapi.socket_fd);
        g_rmapi.socket_fd = -1;
        return B_CONNECTION_REFUSED;
    }
    
    return B_OK;
}

static status_t
rmapi_send_message_socket(rmapi_request *req, rmapi_response *resp)
{
    if (g_rmapi.socket_fd < 0)
        return B_NOT_INITIALIZED;
    
    /* Send request */
    if (send(g_rmapi.socket_fd, req, sizeof(*req), 0) != sizeof(*req))
        return B_IO_ERROR;
    
    /* Receive response */
    if (recv(g_rmapi.socket_fd, resp, sizeof(*resp), 0) != sizeof(*resp))
        return B_IO_ERROR;
    
    return resp->result;
}

/* ============================================================================
 * Generic RMAPI Communication Layer
 * ============================================================================ */

static status_t
rmapi_send_message(rmapi_request *req, rmapi_response *resp)
{
    pthread_mutex_lock(&g_rmapi_lock);
    
    /* Try Haiku port first */
    if (g_rmapi.port >= 0) {
        status_t status = rmapi_send_message_haiku(req, resp);
        pthread_mutex_unlock(&g_rmapi_lock);
        return status;
    }
    
    /* Fall back to socket */
    if (g_rmapi.socket_fd >= 0) {
        status_t status = rmapi_send_message_socket(req, resp);
        pthread_mutex_unlock(&g_rmapi_lock);
        return status;
    }
    
    pthread_mutex_unlock(&g_rmapi_lock);
    return B_NOT_INITIALIZED;
}

/* ============================================================================
 * Public RMAPI Interface Functions
 * ============================================================================ */

/*
 * amd_rmapi_init - Initialize RMAPI connection
 * Called by accelerant init_accelerant()
 */
status_t
amd_rmapi_init(void)
{
    status_t status;
    rmapi_request req = {.cmd = RMAPI_CMD_INIT};
    rmapi_response resp = {};
    
    /* Try Haiku port connection first */
    status = rmapi_connect_haiku_port();
    if (status == B_OK) {
        return rmapi_send_message(&req, &resp);
    }
    
    /* Fall back to socket */
    status = rmapi_connect_socket();
    if (status != B_OK)
        return status;
    
    /* Send init message */
    return rmapi_send_message(&req, &resp);
}

/*
 * amd_rmapi_shutdown - Cleanup RMAPI connection
 */
void
amd_rmapi_shutdown(void)
{
    pthread_mutex_lock(&g_rmapi_lock);
    
    rmapi_request req = {.cmd = RMAPI_CMD_SHUTDOWN};
    rmapi_response resp = {};
    
    rmapi_send_message(&req, &resp);
    
    if (g_rmapi.socket_fd >= 0) {
        close(g_rmapi.socket_fd);
        g_rmapi.socket_fd = -1;
    }
    
    g_rmapi.port = -1;
    
    pthread_mutex_unlock(&g_rmapi_lock);
}

/*
 * amd_get_display_info - Query display information
 * Returns current display configuration from DCE
 */
status_t
amd_get_display_info(int head, void *info, size_t *size)
{
    rmapi_request req = {
        .cmd = RMAPI_CMD_GET_DISPLAY_INFO,
        .param1 = head,
    };
    rmapi_response resp = {};
    
    status_t status = rmapi_send_message(&req, &resp);
    if (status != B_OK)
        return status;
    
    if (info && size) {
        *size = resp.result1;
        if (*size > 0)
            memcpy(info, &resp.result2, *size);  /* Limited copy */
    }
    
    return status;
}

/*
 * amd_set_display_mode - Set display resolution and refresh rate
 * Submits mode change to DCE block
 */
status_t
amd_set_display_mode(int head, const void *mode)
{
    rmapi_request req = {
        .cmd = RMAPI_CMD_SET_DISPLAY_MODE,
        .param1 = head,
        .param2 = (uint64_t)mode,  /* Physical addr or handle */
    };
    rmapi_response resp = {};
    
    return rmapi_send_message(&req, &resp);
}

/*
 * amd_allocate_memory - Allocate GPU memory via GMC
 * Returns handle to GPU buffer
 */
status_t
amd_allocate_memory(size_t size, void **handle)
{
    if (!handle)
        return B_BAD_VALUE;
    
    rmapi_request req = {
        .cmd = RMAPI_CMD_ALLOCATE_MEMORY,
        .param1 = size,
    };
    rmapi_response resp = {};
    
    status_t status = rmapi_send_message(&req, &resp);
    if (status != B_OK)
        return status;
    
    *handle = (void *)resp.result1;
    return B_OK;
}

/*
 * amd_free_memory - Free GPU memory
 */
status_t
amd_free_memory(void *handle)
{
    rmapi_request req = {
        .cmd = RMAPI_CMD_FREE_MEMORY,
        .param1 = (uint64_t)handle,
    };
    rmapi_response resp = {};
    
    return rmapi_send_message(&req, &resp);
}

/*
 * amd_submit_command_buffer - Submit GPU commands
 * Queues commands to GFX ring for execution
 */
status_t
amd_submit_command_buffer(void *cmds, size_t size, void *fence)
{
    rmapi_request req = {
        .cmd = RMAPI_CMD_SUBMIT_COMMAND,
        .param1 = (uint64_t)cmds,
        .param2 = size,
        .param3 = (uint64_t)fence,
    };
    rmapi_response resp = {};
    
    return rmapi_send_message(&req, &resp);
}

/*
 * amd_wait_fence - Wait for GPU command completion
 * Blocks until fence signals or timeout expires
 */
status_t
amd_wait_fence(void *fence, uint32_t timeout_ms)
{
    rmapi_request req = {
        .cmd = RMAPI_CMD_WAIT_FENCE,
        .param1 = (uint64_t)fence,
        .param2 = timeout_ms,
    };
    rmapi_response resp = {};
    
    return rmapi_send_message(&req, &resp);
}

/* ============================================================================
 * Display Control Functions (DCE) - Advanced
 * ============================================================================ */

/*
 * Helper: Build DCE command for mode setting
 * Would be called by amd_set_display_mode
 */
typedef struct {
    uint32_t head;
    uint32_t clock_khz;
    uint16_t h_display, h_sync_start, h_sync_end, h_total;
    uint16_t v_display, v_sync_start, v_sync_end, v_total;
} dce_mode_set_cmd;

/* ============================================================================
 * Graphics (GFX) Command Building - Advanced
 * ============================================================================ */

/*
 * Helper: Build GFX command for rectangle fill
 * Used by fill_rectangle accelerant hook
 */
typedef struct {
    uint32_t opcode;      /* GFX opcode for fill */
    uint32_t color;
    int32_t x, y;
    uint32_t width, height;
} gfx_fill_cmd;

/*
 * Helper: Build fence object for command tracking
 */
typedef struct {
    uint32_t sequence;
    uint32_t timeout_ms;
    volatile uint32_t *fence_addr;
} gpu_fence;

/* ============================================================================
 * Testing/Debug Functions
 * ============================================================================ */

void
rmapi_debug_status(void)
{
    fprintf(stderr, "[RMAPI] Port: %d, Socket: %d\n", g_rmapi.port, g_rmapi.socket_fd);
}

status_t
rmapi_test_connection(void)
{
    rmapi_request req = {.cmd = RMAPI_CMD_INIT};
    rmapi_response resp = {};
    
    return rmapi_send_message(&req, &resp);
}

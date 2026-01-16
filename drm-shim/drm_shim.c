/*
 * DRM Shim for AMDGPU_Abstracted
 *
 * This library emulates libdrm_amdgpu.so to allow Mesa RADV to work with
 * our IPC-based driver instead of requiring a real DRM kernel module.
 *
 * Pattern inspired by nvidia-haiku's DRM compatibility layer.
 */

#include "../src/common/ipc_lib.h"
#include "../src/common/ipc_protocol.h"
#include "amdgpu_drm.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

// Global IPC connection to rmapi_server
static ipc_connection_t g_drm_conn;
static int g_drm_initialized = 0;

/*
 * DRM Version Query
 * RADV calls this first to verify the driver name is "amdgpu"
 */
typedef struct drm_version {
  int version_major;
  int version_minor;
  int version_patchlevel;
  size_t name_len;
  char *name;
  size_t date_len;
  char *date;
  size_t desc_len;
  char *desc;
} drm_version_t;

#define DRM_IOCTL_VERSION _IOWR('d', 0x00, struct drm_version)

int drmGetVersion(int fd, drm_version_t *ver) {
  if (!ver)
    return -1;

  // Return "amdgpu" driver identity
  const char *name = "amdgpu";
  const char *date = "20260115";
  const char *desc = "AMDGPU HIT (Haiku IPC Transport)";

  if (ver->name && ver->name_len >= strlen(name)) {
    strcpy(ver->name, name);
  }
  ver->name_len = strlen(name);

  if (ver->date && ver->date_len >= strlen(date)) {
    strcpy(ver->date, date);
  }
  ver->date_len = strlen(date);

  if (ver->desc && ver->desc_len >= strlen(desc)) {
    strcpy(ver->desc, desc);
  }
  ver->desc_len = strlen(desc);

  ver->version_major = 3;
  ver->version_minor = 57;
  ver->version_patchlevel = 0;

  return 0;
}

/*
 * DRM Command Write/Read
 * Core IPC bridge: translates DRM IOCTLs to our IPC protocol
 */
int drmCommandWriteRead(int fd, unsigned long drmCommandIndex, void *data,
                        unsigned long size) {
  // Initialize IPC connection on first use
  if (!g_drm_initialized) {
    if (ipc_client_connect(HIT_SOCKET_PATH, &g_drm_conn) < 0) {
      fprintf(stderr, "DRM Shim: Failed to connect to rmapi_server\n");
      return -1;
    }
    g_drm_initialized = 1;
  }

  // Map DRM command to IPC message type
  ipc_message_t msg;
  msg.id = 1; // TODO: proper client tracking
  msg.data_size = size;
  msg.data = data;

  switch (drmCommandIndex) {
  case DRM_AMDGPU_GEM_CREATE:
    msg.type = IPC_REQ_ALLOC_MEMORY;
    break;
  case DRM_AMDGPU_GEM_MMAP:
    msg.type = IPC_REQ_GET_GPU_INFO; // Placeholder
    break;
  case DRM_AMDGPU_CS:
    msg.type = IPC_REQ_SUBMIT_COMMAND;
    break;
  case DRM_AMDGPU_INFO:
    msg.type = IPC_REQ_GET_GPU_INFO;
    break;
  default:
    fprintf(stderr, "DRM Shim: Unsupported command 0x%lx\n", drmCommandIndex);
    return -1;
  }

  // Send to rmapi_server
  if (ipc_send_message(&g_drm_conn, &msg) < 0) {
    return -1;
  }

  // Receive reply
  ipc_message_t reply;
  if (ipc_recv_message(&g_drm_conn, &reply) <= 0) {
    return -1;
  }

  // Copy reply data back
  if (reply.data && reply.data_size > 0) {
    memcpy(data, reply.data, reply.data_size < size ? reply.data_size : size);
    free(reply.data);
  }

  return 0;
}

int drmCommandWrite(int fd, unsigned long drmCommandIndex, void *data,
                    unsigned long size) {
  return drmCommandWriteRead(fd, drmCommandIndex, data, size);
}

/*
 * Device Open
 * RADV expects to open /dev/dri/renderD128
 * We intercept this and return a fake FD
 */
int drmOpen(const char *name, const char *busid) {
  // Return a fake FD (we don't actually use it, IPC is socket-based)
  // Just needs to be > 0 to signal success
  return 42; // The answer to everything
}

void drmClose(int fd) {
  // Cleanup IPC connection
  if (g_drm_initialized) {
    ipc_close(&g_drm_conn);
    g_drm_initialized = 0;
  }
}

/*
 * Memory Management Stubs
 * RADV uses these for buffer allocation
 */
int drmPrimeHandleToFD(int fd, uint32_t handle, uint32_t flags, int *prime_fd) {
  // TODO: Implement proper handle→FD mapping
  *prime_fd = handle; // Placeholder
  return 0;
}

int drmPrimeFDToHandle(int fd, int prime_fd, uint32_t *handle) {
  *handle = prime_fd; // Placeholder
  return 0;
}

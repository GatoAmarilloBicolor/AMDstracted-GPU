#include "rmapi.h"
#include "../../kernel-amd/os-primitives/os_primitives.h"
#include "../../src/common/ipc_lib.h"
#include "../../src/common/ipc_protocol.h"
#include "hal.h"
#include <stdlib.h>
#include <string.h>

/*
 * Yo! This is the RMAPI Layer.
 * It's the high-level way to talk to the driver. Apps use this so they don't
 * have to worry about the messy hardware details. It's like the
 * "Settings" menu on your phone—simple and easy.
 *
 * Developed by: Haiku Imposible Team (HIT)
 */

// We keep one main GPU object in memory for everything to share
struct OBJGPU *global_gpu = NULL;

// Turning everything on for the first time
int rmapi_init(void) {
  if (global_gpu)
    return 0; // Already awake!

  os_prim_log("RMAPI: Waking up the driver system...\n");
  global_gpu = os_prim_alloc(sizeof(struct OBJGPU));
  if (!global_gpu)
    return -1; // Big sadness, we ran out of memory!

  memset(global_gpu, 0, sizeof(struct OBJGPU));

  // --- Hardware Discovery (True Abstraction) ---
  // We scan the bus for ANY AMD device (Vendor 0x1002)
  void *pci_handle;
  if (os_prim_pci_find_device(0x1002, 0, &pci_handle) == 0) {
    uint16_t vendor, device;
    os_prim_pci_get_ids(pci_handle, &vendor, &device);

    // We pass this info to the HAL so it can decide how to initialize!
    // The HAL will use the device_id to find the right specialists.
    global_gpu->pci_handle = pci_handle;
    os_prim_log("RMAPI: Found AMD device on the bus. Identifying...\n");
  } else {
    os_prim_log("RMAPI: No AMD hardware found. Using simulation defaults.\n");
  }

  amdgpu_device_init_hal(
      global_gpu); // Starting the especialistas (Specialists)

  os_prim_log("RMAPI: All systems go! Global GPU is live.\n");
  return 0;
}

// Shutting down the whole thing
void rmapi_fini(void) {
  if (global_gpu) {
    amdgpu_device_fini_hal(global_gpu);
    os_prim_free(global_gpu);
    global_gpu = NULL;
  }
  os_prim_log("RMAPI: Driver is going to sleep. See ya!\n");
}

/* --- The Main Commands You'll Use --- */

// 1. "I need some space!" (Allocate memory)
int rmapi_alloc_memory(struct OBJGPU *gpu, size_t size, uint64_t *addr) {
  if (!gpu)
    gpu = global_gpu; // Use the main one if nothing else is given
  if (!gpu)
    return -1;

  os_prim_log("RMAPI: Asking the HAL for some fresh memory space.\n");
  struct amdgpu_buffer buf;
  int ret = amdgpu_buffer_alloc_hal(gpu, size, &buf);
  if (ret == 0) {
    *addr = (uint64_t)buf.cpu_addr; // Giving back the address handle
  }
  return ret;
}

// 2. "I'm done with this space!" (Free memory)
int rmapi_free_memory(struct OBJGPU *gpu, uint64_t addr) {
  if (!gpu)
    gpu = global_gpu;
  if (!gpu)
    return -1;

  os_prim_log("RMAPI: Telling the HAL to clean up this memory spot.\n");
  struct amdgpu_buffer buf = {.cpu_addr = (void *)addr};
  amdgpu_buffer_free_hal(gpu, &buf);
  return 0;
}

// 3. "Yo GPU, do this work!" (Submit command)
int rmapi_submit_command(struct OBJGPU *gpu, struct amdgpu_command_buffer *cb) {
  if (!gpu)
    gpu = global_gpu;
  if (!gpu)
    return -1;

  os_prim_log("RMAPI: Sending a list of jobs to the GPU engine.\n");
  return amdgpu_command_submit_hal(gpu, cb);
}

// 4. "Wait, who ARE you exactly?" (Get GPU info with caching for quality performance)
static struct amdgpu_gpu_info cached_gpu_info;
static int gpu_info_cached = 0;

int rmapi_get_gpu_info(struct OBJGPU *gpu, struct amdgpu_gpu_info *info) {
  if (!gpu)
    gpu = global_gpu;
  if (!gpu)
    return -1;

  // Cache GPU info to avoid repeated HAL calls (quality improvement)
  if (!gpu_info_cached) {
    os_prim_log("RMAPI: Fetching the GPU ID card for you.\n");
    int ret = amdgpu_gpu_get_info_hal(gpu, &cached_gpu_info);
    if (ret == 0) {
      gpu_info_cached = 1;
    } else {
      return ret;
    }
  }
  *info = cached_gpu_info;
  return 0;
}

/* --- Vulkan RMAPI Functions for RADV/Zink Integration --- */

// Persistent IPC connection for Vulkan (performance optimization)
static ipc_connection_t g_vk_conn = {0};
static int g_vk_conn_initialized = 0;

static int ensure_vk_connection() {
  if (!g_vk_conn_initialized) {
    if (ipc_client_connect(HIT_SOCKET_PATH, &g_vk_conn) < 0) {
      os_prim_log("RMAPI: Failed to connect to Vulkan IPC\n");
      return -1;
    }
    g_vk_conn_initialized = 1;
    os_prim_log("RMAPI: Vulkan IPC connection established\n");
  }
  return 0;
}

// Vulkan instance creation
int rmapi_vk_create_instance(void *create_info, void **instance) {
  os_prim_log("RMAPI: Creating Vulkan instance via IPC.\n");

  if (ensure_vk_connection() < 0)
    return -1;

  // Send pointer to create_info, not the struct itself
  ipc_message_t msg = {IPC_REQ_VK_CREATE_INSTANCE, 1, sizeof(void *),
                       &create_info};
  if (ipc_send_message(&g_vk_conn, &msg) != 0)
    return -1;

  ipc_message_t reply;
  if (ipc_recv_message(&g_vk_conn, &reply) <= 0)
    return -1;

  if (reply.type == IPC_REP_VK_CREATE_INSTANCE && reply.data) {
    *instance = *(void **)reply.data;
    os_prim_log("RMAPI: VK instance created: %p\n", *instance);
    free(reply.data);
    return 0;
  }

  if (reply.data)
    free(reply.data);
  return -1;
}

// Enumerate physical devices
int rmapi_vk_enumerate_physical_devices(void *instance, uint32_t *count,
                                        void **devices) {
  os_prim_log("RMAPI: Enumerating Vulkan physical devices.\n");

  if (ensure_vk_connection() < 0)
    return -1;

  ipc_message_t msg = {IPC_REQ_VK_ENUMERATE_PHYSICAL_DEVICES, 1, sizeof(void *),
                       &instance};
  if (ipc_send_message(&g_vk_conn, &msg) != 0)
    return -1;

  ipc_message_t reply;
  if (ipc_recv_message(&g_vk_conn, &reply) <= 0)
    return -1;

  if (reply.type == IPC_REP_VK_ENUMERATE_PHYSICAL_DEVICES && reply.data) {
    // Reply contains count + device list
    struct {
      uint32_t count;
      void *device;
    } *resp = reply.data;
    if (count)
      *count = resp->count;
    if (devices)
      *devices = resp->device;
    os_prim_log("RMAPI: Found %u physical device(s)\n", resp->count);
    free(reply.data);
    return 0;
  }

  if (reply.data)
    free(reply.data);
  return -1;
}

// Create Vulkan device
int rmapi_vk_create_device(void *physical_device, void *create_info,
                           void **device) {
  os_prim_log("RMAPI: Creating Vulkan device.\n");

  if (ensure_vk_connection() < 0)
    return -1;

  // Pack both arguments
  struct {
    void *phys_dev;
    void *create_info;
  } args = {physical_device, create_info};
  ipc_message_t msg = {IPC_REQ_VK_CREATE_DEVICE, 1, sizeof(args), &args};
  if (ipc_send_message(&g_vk_conn, &msg) != 0)
    return -1;

  ipc_message_t reply;
  if (ipc_recv_message(&g_vk_conn, &reply) <= 0)
    return -1;

  if (reply.type == IPC_REP_VK_CREATE_DEVICE && reply.data) {
    *device = *(void **)reply.data;
    os_prim_log("RMAPI: VK device created: %p\n", *device);
    free(reply.data);
    return 0;
  }

  if (reply.data)
    free(reply.data);
  return -1;
}

// Allocate Vulkan memory
int rmapi_vk_allocate_memory(void *device, void *alloc_info, void **memory) {
  os_prim_log("RMAPI: Allocating Vulkan memory.\n");

  if (ensure_vk_connection() < 0)
    return -1;

  struct {
    void *device;
    void *alloc_info;
  } args = {device, alloc_info};
  ipc_message_t msg = {IPC_REQ_VK_ALLOC_MEMORY, 1, sizeof(args), &args};
  if (ipc_send_message(&g_vk_conn, &msg) != 0)
    return -1;

  ipc_message_t reply;
  if (ipc_recv_message(&g_vk_conn, &reply) <= 0)
    return -1;

  if (reply.type == IPC_REP_VK_ALLOC_MEMORY && reply.data) {
    *memory = *(void **)reply.data;
    free(reply.data);
    return 0;
  }

  if (reply.data)
    free(reply.data);
  return -1;
}

// Free Vulkan memory
int rmapi_vk_free_memory(void *device, void *memory) {
  os_prim_log("RMAPI: Freeing Vulkan memory.\n");

  if (ensure_vk_connection() < 0)
    return -1;

  struct {
    void *device;
    void *memory;
  } args = {device, memory};
  ipc_message_t msg = {IPC_REQ_VK_FREE_MEMORY, 1, sizeof(args), &args};
  if (ipc_send_message(&g_vk_conn, &msg) != 0)
    return -1;

  ipc_message_t reply;
  if (ipc_recv_message(&g_vk_conn, &reply) <= 0)
    return -1;

  if (reply.data)
    free(reply.data);
  return (reply.type == IPC_REP_VK_FREE_MEMORY) ? 0 : -1;
}

// Create command pool
int rmapi_vk_create_command_pool(void *device, void *create_info, void **pool) {
  os_prim_log("RMAPI: Creating Vulkan command pool.\n");

  if (ensure_vk_connection() < 0)
    return -1;

  struct {
    void *device;
    void *create_info;
  } args = {device, create_info};
  ipc_message_t msg = {IPC_REQ_VK_CREATE_COMMAND_POOL, 1, sizeof(args), &args};
  if (ipc_send_message(&g_vk_conn, &msg) != 0)
    return -1;

  ipc_message_t reply;
  if (ipc_recv_message(&g_vk_conn, &reply) <= 0)
    return -1;

  if (reply.type == IPC_REP_VK_CREATE_COMMAND_POOL && reply.data) {
    *pool = *(void **)reply.data;
    free(reply.data);
    return 0;
  }

  if (reply.data)
    free(reply.data);
  return -1;
}

// Submit to queue
int rmapi_vk_submit_queue(void *queue, uint32_t submit_count, void *submits,
                          void *fence) {
  os_prim_log("RMAPI: Submitting to Vulkan queue.\n");

  if (ensure_vk_connection() < 0)
    return -1;

  struct {
    void *queue;
    uint32_t count;
    void *submits;
    void *fence;
  } args = {queue, submit_count, submits, fence};
  ipc_message_t msg = {IPC_REQ_VK_SUBMIT_QUEUE, 1, sizeof(args), &args};
  if (ipc_send_message(&g_vk_conn, &msg) != 0)
    return -1;

  ipc_message_t reply;
  if (ipc_recv_message(&g_vk_conn, &reply) <= 0)
    return -1;

  if (reply.data)
    free(reply.data);
  return (reply.type == IPC_REP_VK_SUBMIT_QUEUE) ? 0 : -1;
}
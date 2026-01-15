#include "rmapi.h"
#include "../../kernel-amd/os-primitives/os_primitives.h"
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
static struct OBJGPU *global_gpu = NULL;

// Turning everything on for the first time
int rmapi_init(void) {
  if (global_gpu)
    return 0; // Already awake!

  os_prim_log("RMAPI: Waking up the driver system...\n");
  global_gpu = os_prim_alloc(sizeof(struct OBJGPU));
  if (!global_gpu)
    return -1; // Big sadness, we ran out of memory!

  memset(global_gpu, 0, sizeof(struct OBJGPU));
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

// 4. "Wait, who ARE you exactly?" (Get GPU info)
int rmapi_get_gpu_info(struct OBJGPU *gpu, struct amdgpu_gpu_info *info) {
  if (!gpu)
    gpu = global_gpu;
  if (!gpu)
    return -1;

  os_prim_log("RMAPI: Fetching the GPU ID card for you.\n");
  return amdgpu_gpu_get_info_hal(gpu, info);
}
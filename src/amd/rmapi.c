// RMAPI-style userspace implementation, inspired by NVIDIA
// Allows direct calls from apps, reducing kernel overhead

#include "rmapi.h"
#include "../../kernel-amd/os-primitives/os_primitives.h"
#include <string.h>

static struct OBJGPU *global_gpu = NULL;

int rmapi_init(void) {
  if (global_gpu)
    return 0;
  os_prim_log("RMAPI: Initializing\n");
  global_gpu = os_prim_alloc(sizeof(struct OBJGPU));
  if (!global_gpu)
    return -1;
  memset(global_gpu, 0, sizeof(struct OBJGPU));
  amdgpu_device_init_hal(global_gpu);
  os_prim_log("RMAPI: Initialized global GPU\n");
  return 0;
}

void rmapi_fini(void) {
  if (global_gpu) {
    amdgpu_device_fini_hal(global_gpu);
    os_prim_free(global_gpu);
    global_gpu = NULL;
  }
  os_prim_log("RMAPI: Finalized\n");
}

int rmapi_alloc_memory(struct OBJGPU *gpu, size_t size, uint64_t *addr) {
  if (!gpu)
    gpu = global_gpu;
  if (!gpu)
    return -1;
  os_prim_log("RMAPI: Allocating memory\n");
  struct amdgpu_buffer buf;
  int ret = amdgpu_buffer_alloc_hal(gpu, size, &buf);
  if (ret == 0) {
    *addr = (uint64_t)buf.cpu_addr;
  }
  return ret;
}

int rmapi_free_memory(struct OBJGPU *gpu, uint64_t addr) {
  if (!gpu)
    gpu = global_gpu;
  if (!gpu)
    return -1;
  os_prim_log("RMAPI: Freeing memory\n");
  struct amdgpu_buffer buf = {.cpu_addr = (void *)addr};
  amdgpu_buffer_free_hal(gpu, &buf);
  return 0;
}

int rmapi_submit_command(struct OBJGPU *gpu, struct amdgpu_command_buffer *cb) {
  if (!gpu)
    gpu = global_gpu;
  if (!gpu)
    return -1;
  os_prim_log("RMAPI: Submitting command\n");
  return amdgpu_command_submit_hal(gpu, cb);
}

int rmapi_get_gpu_info(struct OBJGPU *gpu, struct amdgpu_gpu_info *info) {
  if (!gpu)
    gpu = global_gpu;
  if (!gpu)
    return -1;
  os_prim_log("RMAPI: Getting GPU info\n");
  return amdgpu_gpu_get_info_hal(gpu, info);
}
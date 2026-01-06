#ifndef AMD_RMAPI_H
#define AMD_RMAPI_H

#include "hal.h"

// RMAPI-style userspace interface, inspired by NVIDIA
// Allows direct calls from apps to RM, reducing kernel overhead

// RMAPI functions
int rmapi_init(void);
void rmapi_fini(void);
int rmapi_alloc_memory(struct OBJGPU* gpu, size_t size, uint64_t* addr);
int rmapi_free_memory(struct OBJGPU* gpu, uint64_t addr);
int rmapi_submit_command(struct OBJGPU* gpu, struct amdgpu_command_buffer* cb);
int rmapi_get_gpu_info(struct OBJGPU* gpu, struct amdgpu_gpu_info* info);

#endif
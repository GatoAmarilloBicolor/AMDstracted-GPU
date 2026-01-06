// RMAPI-style userspace implementation, inspired by NVIDIA
// Allows direct calls from apps, reducing kernel overhead

#include "rmapi.h"
#include "../../kernel-amd/os-primitives/os_primitives.h"

// Dummy implementations for compilation

static struct OBJGPU* global_gpu = NULL;

int rmapi_init(void) {
    os_prim_log("RMAPI: Initialized\n");
    global_gpu = NULL; // In real, alloc and init
    return 0;
}

void rmapi_fini(void) {
    os_prim_log("RMAPI: Finalized\n");
    global_gpu = NULL;
}

int rmapi_alloc_memory(struct OBJGPU* gpu, size_t size, uint64_t* addr) {
    os_prim_log("RMAPI: Allocating memory\n");
    void* ptr = os_prim_alloc(size);
    *addr = (uint64_t)ptr;
    return ptr ? 0 : -1;
}

int rmapi_free_memory(struct OBJGPU* gpu, uint64_t addr) {
    os_prim_log("RMAPI: Freeing memory\n");
    os_prim_free((void*)addr);
    return 0;
}

int rmapi_submit_command(struct OBJGPU* gpu, struct amdgpu_command_buffer* cb) {
    os_prim_log("RMAPI: Submitting command\n");
    return amdgpu_command_submit_hal(gpu, cb);
}

int rmapi_get_gpu_info(struct OBJGPU* gpu, struct amdgpu_gpu_info* info) {
    os_prim_log("RMAPI: Getting GPU info\n");
    return amdgpu_gpu_get_info_hal(gpu, info);
}
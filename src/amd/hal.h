#ifndef AMD_HAL_H
#define AMD_HAL_H

#include <stdint.h>
#include <stddef.h>
#include <pthread.h>  // For pthreads in Haiku and POSIX

// Hardware Abstraction Layer for AMD GPUs, inspired by NVIDIA

struct amdgpu_gpu_info {
    uint32_t vram_size_mb;
    uint32_t gpu_clock_mhz;
    char gpu_name[32];
};

struct OBJGPU; // Forward declare

typedef struct amdgpu_gpu_info amdgpu_gpu_info_t; // Alias for visibility

// Children objects like NVIDIA RESSERV hierarchy (enhanced)
struct RsResource {
    uint32_t handle;
    struct RsResource* parent;
    struct RsResource** children; // Dynamic array
    int num_children;
    void* data; // Resource-specific data
    pthread_mutex_t lock; // For multi-GPU sync
};

// RESSERV-like functions
struct RsResource* rs_resource_create(uint32_t handle, struct RsResource* parent);
void rs_resource_add_child(struct RsResource* parent, struct RsResource* child);
void rs_resource_destroy(struct RsResource* res);

struct amdgpu_buffer {
    void* cpu_addr;
    uint64_t gpu_addr;
    size_t size;
};

struct amdgpu_command_buffer {
    void* cmds;
    size_t size;
};

struct amdgpu_hal_ops {
    int (*device_init)(struct OBJGPU* adev);
    void (*device_fini)(struct OBJGPU* adev);
    int (*gpu_reset)(struct OBJGPU* adev);
    int (*gpu_get_info)(struct OBJGPU* adev, struct amdgpu_gpu_info* info);
    int (*buffer_alloc)(struct OBJGPU* adev, size_t size, struct amdgpu_buffer* buf);
    void (*buffer_free)(struct OBJGPU* adev, struct amdgpu_buffer* buf);
    int (*command_submit)(struct OBJGPU* adev, struct amdgpu_command_buffer* cb);
    int (*compute_dispatch)(struct OBJGPU* adev, void* kernel, size_t size); // Nuevo: para herencia futura
    // Extensible para OS futuros
};

struct OBJGPU {
    uint32_t asic_type; // E.g., CHIP_NAVI10
    struct amdgpu_hal_ops* hal_ops;
    void* mmio_base;
    struct RsResource* children[5]; // Display, Memory, etc. like NVIDIA
    // Other fields...
};

// HAL dispatch functions
int amdgpu_device_init_hal(struct OBJGPU* adev);
int amdgpu_gpu_get_info_hal(struct OBJGPU* adev, amdgpu_gpu_info_t* info);
int amdgpu_buffer_alloc_hal(struct OBJGPU* adev, size_t size, struct amdgpu_buffer* buf);
void amdgpu_buffer_free_hal(struct OBJGPU* adev, struct amdgpu_buffer* buf);
int amdgpu_command_submit_hal(struct OBJGPU* adev, struct amdgpu_command_buffer* cb);
int amdgpu_compute_dispatch_hal(struct OBJGPU* adev, void* kernel, size_t size);

// ASIC-specific implementations
extern struct amdgpu_hal_ops navi10_hal_ops;
extern struct amdgpu_hal_ops vega10_hal_ops;
// etc.

#endif // AMD_HAL_H
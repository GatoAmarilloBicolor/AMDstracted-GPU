#ifndef AMD_RMAPI_H
#define AMD_RMAPI_H

#include "../hal/hal.h"

// RMAPI-style userspace interface, inspired by NVIDIA
// Allows direct calls from apps to RM, reducing kernel overhead

// RMAPI functions
int rmapi_init(void);
void rmapi_fini(void);
int rmapi_alloc_memory(struct OBJGPU* gpu, size_t size, uint64_t* addr);
int rmapi_free_memory(struct OBJGPU* gpu, uint64_t addr);
int rmapi_submit_command(struct OBJGPU* gpu, struct amdgpu_command_buffer* cb);
int rmapi_get_gpu_info(struct OBJGPU* gpu, struct amdgpu_gpu_info* info);

// Display & Mode Setting - disabled due to header issues
// #ifdef __HAIKU__
// #include <GraphicsDefs.h>  // For display_mode
// int rmapi_set_display_mode(struct OBJGPU* gpu, const display_mode* mode);
// #else
// struct display_mode;
// int rmapi_set_display_mode(struct OBJGPU* gpu, const struct display_mode* mode);
// #endif

// Vulkan RMAPI functions (for RADV/Zink integration)
int rmapi_vk_create_instance(void* create_info, void** instance);
int rmapi_vk_enumerate_physical_devices(void* instance, uint32_t* count, void** devices);
int rmapi_vk_create_device(void* physical_device, void* create_info, void** device);
int rmapi_vk_allocate_memory(void* device, void* alloc_info, void** memory);
int rmapi_vk_free_memory(void* device, void* memory);
int rmapi_vk_create_command_pool(void* device, void* create_info, void** pool);
int rmapi_vk_submit_queue(void* queue, uint32_t submit_count, void* submits, void* fence);

// OpenGL RMAPI functions (for direct OpenGL support)
typedef void* rmapi_gl_context;

int rmapi_gl_init(void);
rmapi_gl_context* rmapi_gl_create_context(void);
int rmapi_gl_make_current(rmapi_gl_context* ctx);
int rmapi_gl_swap_buffers(rmapi_gl_context* ctx);
void rmapi_gl_destroy_context(rmapi_gl_context* ctx);
int rmapi_gl_create_program(const char* vertex_src, const char* fragment_src, unsigned int* program);
int rmapi_gl_create_buffer(size_t size, const void* data, unsigned int* buffer);
int rmapi_gl_create_texture(int width, int height, unsigned int format, const void* data, unsigned int* texture);
int rmapi_gl_draw_arrays(unsigned int mode, int count);
void rmapi_gl_fini(void);

// Get current GPU instance
struct OBJGPU *rmapi_get_gpu(void);

#endif
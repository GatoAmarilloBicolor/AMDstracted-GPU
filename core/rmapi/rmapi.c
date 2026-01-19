#include "rmapi.h"
#include "../../os/os_interface.h"
#include "../ipc/ipc_lib.h"
#include "../ipc/ipc_protocol.h"
#include "../hal/hal.h"
#include <stdlib.h>
#include <string.h>

#ifdef __HAIKU__
#include <GraphicsDefs.h>
#endif

// Macros for OS calls
#define os_prim_log os_get_interface()->log
#define os_prim_alloc os_get_interface()->alloc
#define os_prim_free os_get_interface()->free
#define os_prim_pci_find_device os_get_interface()->prim_pci_find_device
#define os_prim_pci_get_ids os_get_interface()->prim_pci_get_ids

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

  amdgpu_device_init_hal(global_gpu); // Starting the especialistas (Specialists)

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

  // For real hardware: Use DRM GEM allocation
  // This would call drmIoctl(drm_fd, DRM_IOCTL_AMDGPU_GEM_CREATE, &args)
  // and return the GPU virtual address

  // Placeholder for real implementation
  *addr = (uint64_t)os_prim_alloc(size); // Temporary - should use DRM
  return 0;
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
    cached_gpu_info.device_id = gpu->device_id;
    cached_gpu_info.family = gpu->family;
    cached_gpu_info.asic_type = gpu->asic_type;
    cached_gpu_info.vram_size_mb = 1024; // Placeholder
    cached_gpu_info.gpu_clock_mhz = 1500; // Placeholder
    strcpy(cached_gpu_info.gpu_name, "AMD GPU"); // Placeholder
    cached_gpu_info.vram_base = 0; // Placeholder
    gpu_info_cached = 1;
  }

  *info = cached_gpu_info;
  return 0;
}

// 5. Create buffer object
int rmapi_create_buffer(struct OBJGPU *gpu, size_t size, uint32_t usage, struct amdgpu_buffer **buffer) {
    (void)usage;
  if (!gpu)
    gpu = global_gpu;
  if (!gpu || !buffer)
    return -1;

  *buffer = os_prim_alloc(sizeof(struct amdgpu_buffer));
  if (!*buffer)
    return -1;

  if (amdgpu_buffer_alloc_hal(gpu, size, *buffer) != 0) {
    os_prim_free(*buffer);
    return -1;
  }

  return 0;
}

// 6. Destroy buffer object
int rmapi_destroy_buffer(struct OBJGPU *gpu, struct amdgpu_buffer *buffer) {
  if (!gpu)
    gpu = global_gpu;
  if (!gpu || !buffer)
    return -1;

  amdgpu_buffer_free_hal(gpu, buffer);
  os_prim_free(buffer);
  return 0;
}

// 7. Create command buffer
int rmapi_create_command_buffer(struct OBJGPU *gpu, struct amdgpu_command_buffer **cb) {
  if (!gpu)
    gpu = global_gpu;
  if (!gpu || !cb)
    return -1;

  *cb = os_prim_alloc(sizeof(struct amdgpu_command_buffer));
  if (!*cb)
    return -1;

  memset(*cb, 0, sizeof(struct amdgpu_command_buffer));
  (*cb)->gpu = gpu;

  return 0;
}

// 8. Destroy command buffer
int rmapi_destroy_command_buffer(struct OBJGPU *gpu, struct amdgpu_command_buffer *cb) {
  if (!gpu)
    gpu = global_gpu;
  if (!gpu || !cb)
    return -1;

  os_prim_free(cb);
  return 0;
}

// Vulkan stubs (for future RADV integration)
int rmapi_vk_create_instance(void* create_info, void** instance) {
    (void)create_info;
  // Stub
  *instance = NULL;
  return -1;
}

int rmapi_vk_enumerate_physical_devices(void* instance, uint32_t* count, void** devices) {
    (void)instance; (void)devices;
  // Stub
  *count = 0;
  return -1;
}

int rmapi_vk_create_device(void* physical_device, void* create_info, void** device) {
    (void)physical_device; (void)create_info;
  // Stub
  *device = NULL;
  return -1;
}

int rmapi_vk_allocate_memory(void* device, void* alloc_info, void** memory) {
    (void)device; (void)alloc_info;
  // Stub
  *memory = NULL;
  return -1;
}

int rmapi_vk_free_memory(void* device, void* memory) {
    (void)device; (void)memory;
  // Stub
  return -1;
}

int rmapi_vk_create_command_pool(void* device, void* create_info, void** pool) {
    (void)device; (void)create_info;
  // Stub
  *pool = NULL;
  return -1;
}

int rmapi_vk_submit_queue(void* queue, uint32_t submit_count, void* submits, void* fence) {
    (void)queue; (void)submit_count; (void)submits; (void)fence;
  // Stub
  return -1;
}

// OpenGL RMAPI functions (for direct OpenGL support on R600)
int rmapi_gl_init(void) {
    // Initialize OpenGL for R600 - no Vulkan
    return 0;
}

rmapi_gl_context* rmapi_gl_create_context(void) {
    // Create OpenGL context for R600
    rmapi_gl_context *ctx = os_prim_alloc(sizeof(void*));
    if (!ctx) return NULL;
    // Stub - implement R600 context creation
    return ctx;
}

int rmapi_gl_make_current(rmapi_gl_context* ctx) {
    if (!ctx) return -1;
    // Make R600 context current
    return 0;
}

int rmapi_gl_swap_buffers(rmapi_gl_context* ctx) {
    if (!ctx) return -1;
    // Swap buffers on R600
    return 0;
}

void rmapi_gl_destroy_context(rmapi_gl_context* ctx) {
    if (!ctx) return;
    os_prim_free(ctx);
}

int rmapi_gl_create_program(const char* vertex_src, const char* fragment_src, unsigned int* program) {
    if (!program) return -1;
    // Compile GLSL to R600 ISA using adapted shader compiler
    // Stub - implement R600 shader compilation
    *program = 1;  // Dummy
    return 0;
}

int rmapi_gl_create_buffer(size_t size, const void* data, unsigned int* buffer) {
    if (!buffer) return -1;
    // Create buffer on R600
    *buffer = 1;  // Dummy
    return 0;
}

int rmapi_gl_create_texture(int width, int height, unsigned int format, const void* data, unsigned int* texture) {
    if (!texture) return -1;
    // Create texture on R600
    *texture = 1;  // Dummy
    return 0;
}

int rmapi_gl_draw_arrays(unsigned int mode, int count) {
    // Draw on R600 using VLIW commands from r600_blit.c logic
    return 0;
}

void rmapi_gl_fini(void) {
    // Finalize R600 OpenGL
}


// Get current GPU instance
struct OBJGPU *rmapi_get_gpu(void) {
  return global_gpu;
}
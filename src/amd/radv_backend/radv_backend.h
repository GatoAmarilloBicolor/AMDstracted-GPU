/*
 * RADV Backend Wrapper - Agnóstic Vulkan Integration
 * 
 * Provides Vulkan device interface wrapping RMAPI
 * Compatible with RADV driver on Linux, Haiku, FreeBSD
 * 
 * Developed by: Haiku Imposible Team (HIT)
 */

#ifndef RADV_BACKEND_H
#define RADV_BACKEND_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================================
 * VULKAN TYPES (Minimal, agnóstic)
 * ============================================================================ */

typedef uint32_t VkResult;
typedef uint64_t VkDevice;
typedef uint64_t VkPhysicalDevice;
typedef uint64_t VkInstance;
typedef uint64_t VkQueue;
typedef uint64_t VkCommandBuffer;
typedef uint64_t VkBuffer;
typedef uint64_t VkImage;
typedef uint64_t VkMemory;

#define VK_SUCCESS 0
#define VK_ERROR_DEVICE_LOST 1
#define VK_ERROR_OUT_OF_DEVICE_MEMORY 2

/* ============================================================================
 * PHYSICAL DEVICE INFO
 * ============================================================================ */

typedef struct {
    char name[256];
    uint32_t vendor_id;
    uint32_t device_id;
    uint32_t driver_version;
    uint64_t total_memory;
    uint32_t max_workgroup_size[3];
    uint32_t max_image_dimension[3];
} radv_physical_device_info_t;

/* ============================================================================
 * DEVICE CREATION
 * ============================================================================ */

typedef struct {
    uint32_t queue_count;
    uint32_t queue_family;
    float queue_priorities[8];
} radv_device_create_info_t;

/* ============================================================================
 * BUFFER & MEMORY
 * ============================================================================ */

typedef struct {
    size_t size;
    uint32_t usage_flags;
    uint32_t memory_type;
} radv_buffer_create_info_t;

typedef enum {
    RADV_MEMORY_TYPE_HOST_VISIBLE = 0x1,
    RADV_MEMORY_TYPE_DEVICE_LOCAL = 0x2,
    RADV_MEMORY_TYPE_COHERENT = 0x4,
} radv_memory_type_t;

/* ============================================================================
 * COMMAND BUFFER
 * ============================================================================ */

typedef enum {
    RADV_COMMAND_BUFFER_LEVEL_PRIMARY = 0,
    RADV_COMMAND_BUFFER_LEVEL_SECONDARY = 1,
} radv_command_buffer_level_t;

typedef struct {
    uint32_t level;
    uint32_t command_pool;
} radv_command_buffer_allocate_info_t;

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

/**
 * Initialize RADV backend
 * 
 * @return VK_SUCCESS on success
 */
VkResult radv_init(void);

/**
 * Enumerate physical devices
 * 
 * @param devices       Output array of physical devices
 * @param device_count  Input: max devices, Output: actual count
 * @return VK_SUCCESS on success
 */
VkResult radv_enumerate_devices(VkPhysicalDevice *devices, uint32_t *device_count);

/**
 * Get device properties
 */
VkResult radv_get_device_info(VkPhysicalDevice device, radv_physical_device_info_t *info);

/**
 * Create logical device
 */
VkResult radv_create_device(VkPhysicalDevice physical_device,
                            const radv_device_create_info_t *create_info,
                            VkDevice *device);

/**
 * Destroy device
 */
void radv_destroy_device(VkDevice device);

/**
 * Create buffer
 */
VkResult radv_create_buffer(VkDevice device,
                           const radv_buffer_create_info_t *create_info,
                           VkBuffer *buffer);

/**
 * Allocate memory
 */
VkResult radv_allocate_memory(VkDevice device, size_t size, uint32_t memory_type,
                             VkMemory *memory);

/**
 * Map memory to CPU
 */
VkResult radv_map_memory(VkDevice device, VkMemory memory, void **ppData);

/**
 * Unmap memory
 */
void radv_unmap_memory(VkDevice device, VkMemory memory);

/**
 * Allocate command buffer
 */
VkResult radv_allocate_command_buffer(VkDevice device,
                                      const radv_command_buffer_allocate_info_t *allocate_info,
                                      VkCommandBuffer *cmd_buffer);

/**
 * Begin command buffer recording
 */
VkResult radv_begin_command_buffer(VkCommandBuffer cmd_buffer);

/**
 * End command buffer recording
 */
VkResult radv_end_command_buffer(VkCommandBuffer cmd_buffer);

/**
 * Submit command buffer to queue
 */
VkResult radv_queue_submit(VkQueue queue, VkCommandBuffer cmd_buffer);

/**
 * Wait for device idle
 */
VkResult radv_device_wait_idle(VkDevice device);

/**
 * Shutdown RADV backend
 */
void radv_fini(void);

#endif // RADV_BACKEND_H

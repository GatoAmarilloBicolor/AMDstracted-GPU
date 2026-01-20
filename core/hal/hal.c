#include "hal.h"
#include "../../os/os_interface.h"
#include "../../drivers/interface/mmio_access.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>

// DRM includes for real communication
#ifdef __linux__
#include <libdrm/drm.h>
#include <libdrm/amdgpu_drm.h>
#endif

// Fallback for non-Linux systems
#ifndef DRM_IOCTL_AMDGPU_GEM_CREATE
#define DRM_IOCTL_AMDGPU_GEM_CREATE 0x00 // Placeholder
#endif

// Fallback for O_CLOEXEC
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

// DRM ioctl definitions (fallback for systems without libdrm)
#ifndef DRM_IOCTL_GEM_CREATE
#define DRM_IOCTL_GEM_CREATE 0xc0206400
#endif

#ifndef DRM_IOCTL_GEM_MMAP
#define DRM_IOCTL_GEM_MMAP 0xc0206402
#endif

#ifndef DRM_IOCTL_GEM_CLOSE
#define DRM_IOCTL_GEM_CLOSE 0xc0106401
#endif

// DRM structures (agnostic) - only define if not available
#ifndef DRM_GEM_CLOSE
union hal_drm_gem_create {
    struct {
        uint64_t size;
        uint32_t flags;
        uint32_t handle;
    } in;
};

union hal_drm_gem_mmap {
    struct {
        uint32_t handle;
        uint32_t pad;
        uint64_t offset;
    } in;
};

struct hal_drm_gem_close {
    uint32_t handle;
};
#endif

// Macros for OS calls
#define os_prim_log os_get_interface()->log
#define os_prim_alloc os_get_interface()->alloc
#define os_prim_free os_get_interface()->free
#define os_prim_delay_us os_get_interface()->delay_us

// DRM communication state
static int drm_fd = -1;
static int drm_real_mode = 0;  // 0=simulation, 1=real DRM

// Forward declarations for IP blocks
extern struct ip_block_ops gmc_v10_ip_block;
extern struct ip_block_ops r600_ip_block;
extern struct ip_block_ops dce_v10_ip_block;
extern struct ip_block_ops dcn_v1_ip_block;

// DRM communication functions
static int drm_open_device(const char *device_path);
static void drm_close_device(void);
static int drm_is_real_available(void);

// IP Block registration
int ip_block_register(struct OBJGPU *adev, struct ip_block_ops *block) {
    if (!adev || !block || adev->num_ip_blocks >= AMDGPU_MAX_IP_BLOCKS) {
        return -1;
    }
    adev->ip_blocks[adev->num_ip_blocks++] = block;

    // Also register in handler if exists
    if (adev->handler) {
        adev->handler->register_ip_block(adev->handler, block);
    }
    return 0;
}

// AMD GPU Handler implementation
static int amd_gpu_handler_init_hardware(struct amd_gpu_handler *handler) {
    // Call IP block initialization sequence - delegate to real IP blocks
    for (int i = 0; i < handler->num_ip_blocks; i++) {
        struct ip_block_ops *block = handler->ip_blocks[i];
        if (block->early_init && block->early_init(handler->gpu) != 0) {
            os_prim_log("Handler: Early init failed for %s\n", block->name);
            return -1;
        }
    }

    for (int i = 0; i < handler->num_ip_blocks; i++) {
        struct ip_block_ops *block = handler->ip_blocks[i];
        if (block->sw_init && block->sw_init(handler->gpu) != 0) {
            os_prim_log("Handler: SW init failed for %s\n", block->name);
            return -1;
        }
    }

    for (int i = 0; i < handler->num_ip_blocks; i++) {
        struct ip_block_ops *block = handler->ip_blocks[i];
        if (block->hw_init && block->hw_init(handler->gpu) != 0) {
            os_prim_log("Handler: HW init failed for %s\n", block->name);
            return -1;
        }
    }

    for (int i = 0; i < handler->num_ip_blocks; i++) {
        struct ip_block_ops *block = handler->ip_blocks[i];
        if (block->late_init && block->late_init(handler->gpu) != 0) {
            os_prim_log("Handler: Late init failed for %s\n", block->name);
            return -1;
        }
    }

    os_prim_log("Handler: Hardware initialization complete - all IP blocks initialized\n");
    return 0;
}

static int amd_gpu_handler_fini_hardware(struct amd_gpu_handler *handler) {
    for (int i = handler->num_ip_blocks - 1; i >= 0; i--) {
        struct ip_block_ops *block = handler->ip_blocks[i];
        if (block->hw_fini && block->hw_fini(handler->gpu) != 0) {
            return -1;
        }
    }

    for (int i = handler->num_ip_blocks - 1; i >= 0; i--) {
        struct ip_block_ops *block = handler->ip_blocks[i];
        if (block->sw_fini && block->sw_fini(handler->gpu) != 0) {
            return -1;
        }
    }

    os_prim_log("Handler: Hardware finalization complete\n");
    return 0;
}

static bool amd_gpu_handler_is_hardware_idle(struct amd_gpu_handler *handler) {
    for (int i = 0; i < handler->num_ip_blocks; i++) {
        struct ip_block_ops *block = handler->ip_blocks[i];
        if (block->is_idle && !block->is_idle(handler->gpu)) {
            return false;
        }
    }
    return true;
}

static int amd_gpu_handler_wait_for_idle(struct amd_gpu_handler *handler) {
    for (int i = 0; i < handler->num_ip_blocks; i++) {
        struct ip_block_ops *block = handler->ip_blocks[i];
        if (block->wait_for_idle && block->wait_for_idle(handler->gpu) != 0) {
            return -1;
        }
    }
    return 0;
}

static int amd_gpu_handler_register_ip_block(struct amd_gpu_handler *handler, struct ip_block_ops *block) {
    if (!handler || !block || handler->num_ip_blocks >= AMDGPU_MAX_IP_BLOCKS) {
        return -1;
    }
    handler->ip_blocks[handler->num_ip_blocks++] = block;
    os_prim_log("Handler: Registered IP block %s\n", block->name);
    return 0;
}

static struct ip_block_ops *amd_gpu_handler_find_ip_block(struct amd_gpu_handler *handler, const char *name) {
    for (int i = 0; i < handler->num_ip_blocks; i++) {
        if (strcmp(handler->ip_blocks[i]->name, name) == 0) {
            return handler->ip_blocks[i];
        }
    }
    return NULL;
}

// Create GPU handler
struct amd_gpu_handler *amd_gpu_handler_create(struct OBJGPU *gpu) {
    struct amd_gpu_handler *handler = os_prim_alloc(sizeof(struct amd_gpu_handler));
    if (!handler) {
        return NULL;
    }

    memset(handler, 0, sizeof(struct amd_gpu_handler));
    handler->gpu = gpu;

    // Set function pointers
    handler->init_hardware = amd_gpu_handler_init_hardware;
    handler->fini_hardware = amd_gpu_handler_fini_hardware;
    handler->is_hardware_idle = amd_gpu_handler_is_hardware_idle;
    handler->wait_for_idle = amd_gpu_handler_wait_for_idle;
    handler->register_ip_block = amd_gpu_handler_register_ip_block;
    handler->find_ip_block = amd_gpu_handler_find_ip_block;

    gpu->handler = handler;
    os_prim_log("Handler: GPU handler created\n");
    return handler;
}

// Destroy GPU handler
void amd_gpu_handler_destroy(struct amd_gpu_handler *handler) {
    if (handler) {
        os_prim_free(handler);
    }
}

// DRM communication implementation
static int drm_open_device(const char *device_path) {
    if (drm_fd >= 0) {
        os_prim_log("[HAL] DRM device already open\n");
        return 0;
    }

    drm_fd = open(device_path, O_RDWR | O_CLOEXEC);
    if (drm_fd < 0) {
        os_prim_log("[HAL] Failed to open DRM device %s: %m\n", device_path);
        return -1;
    }

    os_prim_log("[HAL] DRM device opened: %s (fd=%d)\n", device_path, drm_fd);
    drm_real_mode = 1;
    return 0;
}

static void drm_close_device(void) {
    if (drm_fd >= 0) {
        close(drm_fd);
        drm_fd = -1;
        drm_real_mode = 0;
        os_prim_log("[HAL] DRM device closed\n");
    }
}

static int drm_is_real_available(void) {
    // Try to open DRM device to check availability
    int test_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (test_fd >= 0) {
        close(test_fd);
        return 1;
    }
    return 0;
}

// AMD GPU device initialization
int amdgpu_device_init_hal(struct OBJGPU *adev) {
    os_prim_log("HAL: Initializing AMD GPU device...\n");

    // Try to open DRM device for real hardware access
    if (drm_open_device("/dev/dri/card0") == 0) {
        os_prim_log("HAL: ✅ Real DRM communication enabled!\n");
    } else {
        os_prim_log("HAL: ⚠️  DRM device not accessible, using simulation mode\n");
        os_prim_log("HAL: 💡 For real acceleration: ensure /dev/dri/card0 exists and is accessible\n");
    }

    // Create GPU handler
    struct amd_gpu_handler *handler = amd_gpu_handler_create(adev);
    if (!handler) {
        os_prim_log("HAL: Failed to create GPU handler\n");
        drm_close_device();
        return -1;
    }

    // Initialize MMIO access (try real, fallback to simulation)
    if (mmio_init(adev->pci_handle, &adev->mmio_base, &adev->mmio_size) != 0) {
        os_prim_log("HAL: MMIO access failed, using simulation\n");
        // Still continue - MMIO is optional for basic operation
    }

    // Register IP blocks with handler
    if (handler->register_ip_block(handler, &gmc_v10_ip_block) != 0 ||
        handler->register_ip_block(handler, &r600_ip_block) != 0 ||
        handler->register_ip_block(handler, &dce_v10_ip_block) != 0 ||
        handler->register_ip_block(handler, &dcn_v1_ip_block) != 0) {
        os_prim_log("HAL: Failed to register IP blocks\n");
        drm_close_device();
        return -1;
    }

    // Initialize hardware through handler
    if (handler->init_hardware(handler) != 0) {
        os_prim_log("HAL: Hardware initialization failed\n");
        drm_close_device();
        return -1;
    }

    if (drm_real_mode) {
        os_prim_log("HAL: 🎯 AMD GPU device initialized with REAL DRM acceleration!\n");
    } else {
        os_prim_log("HAL: 🎭 AMD GPU device initialized in SIMULATION mode\n");
    }

    return 0;
}

// AMD GPU device finalization
void amdgpu_device_fini_hal(struct OBJGPU *adev) {
    // Close DRM device first
    drm_close_device();

    if (adev->handler) {
        amd_gpu_handler_destroy(adev->handler);
        adev->handler = NULL;
    }

    if (adev->mmio_base) {
        mmio_fini(adev->mmio_base, adev->mmio_size);
        adev->mmio_base = 0;
        adev->mmio_size = 0;
    }

    os_prim_log("HAL: AMD GPU device finalized\n");
}

// GPU info retrieval
int amdgpu_gpu_get_info_hal(struct OBJGPU *adev, amdgpu_gpu_info_t *info) {
    if (!adev || !info) {
        return -1;
    }

    // Fill basic info
    info->device_id = adev->device_id;
    info->family = adev->family;
    info->asic_type = adev->asic_type;
    info->vram_size_mb = 4096; // Placeholder
    info->gpu_clock_mhz = 1500; // Placeholder
    strcpy(info->gpu_name, "AMD Radeon RX"); // Placeholder
    info->vram_base = 0; // Placeholder

    return 0;
}

// Buffer allocation with DRM support (agnostic approach)
int amdgpu_buffer_alloc_hal(struct OBJGPU *adev, size_t size, struct amdgpu_buffer *buf) {
    if (!adev || !buf) {
        return -1;
    }

    buf->size = size;

    if (drm_real_mode && drm_fd >= 0) {
        // REAL DRM: Try GEM buffer allocation (Linux/Haiku agnostic)
        os_prim_log("HAL: 📡 Attempting real GEM buffer allocation (size: %zu)\n", size);

        // Use generic DRM GEM create (works on most systems)
        union hal_drm_gem_create create_args = {.in.size = size, .in.flags = 0};

        if (ioctl(drm_fd, DRM_IOCTL_GEM_CREATE, &create_args) == 0) {
            buf->handle = create_args.in.handle;

            // Try to map to CPU space
            union hal_drm_gem_mmap mmap_args = {.in.handle = buf->handle};

            if (ioctl(drm_fd, DRM_IOCTL_GEM_MMAP, &mmap_args) == 0) {
                buf->cpu_addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,
                                   drm_fd, mmap_args.in.offset);
                if (buf->cpu_addr != MAP_FAILED) {
                    buf->gpu_addr = 0; // Real GPU address not available in userspace
                    os_prim_log("HAL: ✅ Real GEM buffer allocated (handle: %u, addr: %p)\n",
                               buf->handle, buf->cpu_addr);
                    return 0;
                } else {
                    os_prim_log("HAL: ❌ GEM mmap failed, cleaning up\n");
                    // Clean up handle
                    struct hal_drm_gem_close close_args = {.handle = buf->handle};
                    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_args);
                }
            } else {
                os_prim_log("HAL: ❌ GEM mmap ioctl failed\n");
            }
        } else {
            os_prim_log("HAL: ❌ GEM create ioctl failed (errno: %d), falling back to simulation\n", errno);
        }
    }

    // SIMULATION FALLBACK: Use OS allocation when DRM fails
    os_prim_log("HAL: 🎭 Using simulation buffer allocation (size: %zu)\n", size);

    buf->cpu_addr = os_prim_alloc(size);
    if (!buf->cpu_addr) {
        os_prim_log("HAL: ❌ Simulation allocation failed\n");
        return -1;
    }

    buf->gpu_addr = (uint64_t)buf->cpu_addr; // Fake GPU address for simulation
    buf->handle = (uint32_t)(uintptr_t)buf->cpu_addr; // Fake handle

    os_prim_log("HAL: ✅ Simulation buffer allocated (addr: %p)\n", buf->cpu_addr);
    return 0;
}

// Buffer free with DRM support
void amdgpu_buffer_free_hal(struct OBJGPU *adev, struct amdgpu_buffer *buf) {
    (void)adev;
    if (!buf) return;

    if (drm_real_mode && drm_fd >= 0 && buf->handle > 0) {
        // REAL DRM: Clean up GEM buffer
        os_prim_log("HAL: 📡 Freeing real GEM buffer (handle: %u)\n", buf->handle);

        // Unmap first if mapped
        if (buf->cpu_addr && buf->cpu_addr != MAP_FAILED) {
            munmap(buf->cpu_addr, buf->size);
        }

        // Close GEM handle
        struct hal_drm_gem_close close_args = {.handle = buf->handle};
        ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_args);

        os_prim_log("HAL: ✅ Real GEM buffer freed\n");
    } else if (buf->cpu_addr) {
        // SIMULATION: Free allocated memory
        os_prim_log("HAL: 🎭 Freeing simulation buffer\n");
        os_prim_free(buf->cpu_addr);
    }

    // Clear buffer structure
    memset(buf, 0, sizeof(*buf));
}

// Command submission
int amdgpu_command_submit_hal(struct OBJGPU *adev, struct amdgpu_command_buffer *cb) {
    if (!adev || !cb) {
        return -1;
    }

    // For now, just log - real implementation would submit to ring
    os_prim_log("HAL: Command buffer submitted (%zu bytes)\n", cb->size);

    return 0;
}

// Reset
int amdgpu_hal_reset(struct OBJGPU *adev) {
    (void)adev;
    os_prim_log("HAL: GPU reset requested\n");
    // Placeholder for reset logic
    return 0;
}

// Heartbeat
void *amdgpu_hal_heartbeat(void *arg) {
    struct OBJGPU *adev = arg;
    (void)adev;
    os_prim_log("HAL: Heartbeat thread started\n");

    while (1) {
        // Check GPU health
        os_prim_delay_us(1000000); // 1 second
    }

    return NULL;
}
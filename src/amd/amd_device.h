#ifndef AMD_DEVICE_H
#define AMD_DEVICE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Forward declaration for HAL integration */
typedef struct OBJGPU OBJGPU;

/* GPU Generations */
typedef enum {
    AMD_VLIW,       /* VLIW4/2 (2005-2012) */
    AMD_GCN1,       /* SI (2012-2013) */
    AMD_GCN2,       /* CIK (2013-2014) */
    AMD_GCN3,       /* Fiji/Polaris (2015) */
    AMD_GCN4,       /* Vega (2016) */
    AMD_GCN5,       /* RDNA Gen 1 (2019) */
    AMD_RDNA2,      /* RDNA2 (2020-2021) */
    AMD_RDNA3,      /* RDNA3 (2022+) */
    AMD_UNKNOWN,
} amd_gpu_generation_t;

/* Backend Types */
typedef enum {
    AMD_BACKEND_RADV,       /* Vulkan (modern GPUs) */
    AMD_BACKEND_MESA,       /* Mesa OpenGL (legacy) */
    AMD_BACKEND_SOFTWARE,   /* LLVMPipe (fallback) */
} amd_backend_type_t;

/* GPU Capabilities */
typedef struct {
    bool has_radv;
    bool has_mesa;
    bool has_compute;
    bool has_raytracing;
    bool has_display;
} amd_gpu_capabilities_t;

/* GPU Device Info */
typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t revision;
    
    amd_gpu_generation_t generation;
    const char *codename;
    const char *marketing_name;
    
    uint32_t max_compute_units;
    uint32_t max_wave64_per_cu;
    uint32_t max_vram_mb;
    
    amd_gpu_capabilities_t capabilities;
    amd_backend_type_t preferred_backend;
} amd_gpu_device_info_t;

/* IP Block structure for real hardware access */
typedef struct {
    const char *name;
    uint32_t version;
    int (*early_init)(OBJGPU *gpu);
    int (*hw_init)(OBJGPU *gpu);
    int (*hw_fini)(OBJGPU *gpu);
    int (*sw_init)(OBJGPU *gpu);
    int (*sw_fini)(OBJGPU *gpu);
} amd_ip_block_ops_t;

/* Forward declarations */
typedef struct amd_device amd_device_t;
typedef struct amd_gpu_handler amd_gpu_handler_t;
typedef struct amd_api_backend amd_api_backend_t;
typedef struct amd_platform_bridge amd_platform_bridge_t;

/* GPU Handler: Generation-specific operations with IP block support */
typedef struct amd_gpu_handler {
    const char *name;
    amd_gpu_generation_t generation;
    
    /* IP Block Members (for direct hardware access) */
    struct {
        amd_ip_block_ops_t *gmc;      /* Graphics Memory Controller */
        amd_ip_block_ops_t *gfx;      /* Graphics Engine */
        amd_ip_block_ops_t *sdma;     /* DMA Engines */
        amd_ip_block_ops_t *display;  /* Display Engine */
        amd_ip_block_ops_t *clock;    /* Clock/Power Management */
    } ip_blocks;
    
    /* Core initialization - now delegates to IP blocks */
    int (*init_hardware)(amd_device_t *dev);      /* NEW: calls real IP blocks */
    int (*hw_init)(amd_device_t *dev);            /* LEGACY: for compatibility */
    int (*hw_fini)(amd_device_t *dev);
    
    int (*init_ip_blocks)(amd_device_t *dev);
    int (*enable_ip_block)(amd_device_t *dev, int block_type);
    int (*cleanup_ip_blocks)(amd_device_t *dev);
    
    int (*init_gmc)(amd_device_t *dev);
    int (*allocate_vram)(amd_device_t *dev, size_t size, uint64_t *gpu_addr);
    int (*free_vram)(amd_device_t *dev, uint64_t gpu_addr);
    
    int (*init_gfx)(amd_device_t *dev);
    int (*submit_command)(amd_device_t *dev, void *cmd);
    
    int (*init_display)(amd_device_t *dev);
    int (*set_display_mode)(amd_device_t *dev, uint32_t width, uint32_t height);
    
    void (*cleanup)(amd_device_t *dev);
} amd_gpu_handler_t;

/* API Backend: RADV vs Mesa abstraction */
typedef struct amd_api_backend {
    const char *name;
    amd_backend_type_t type;
    
    int (*initialize)(amd_device_t *dev);
    int (*create_buffer)(amd_device_t *dev, size_t size, void **buffer);
    int (*bind_texture)(amd_device_t *dev, void *texture);
    int (*draw_primitives)(amd_device_t *dev, void *draw_call);
    int (*submit_compute)(amd_device_t *dev, void *kernel);
    void (*cleanup)(amd_device_t *dev);
} amd_api_backend_t;

/* Platform Bridge: OS-specific abstraction */
typedef struct amd_platform_bridge {
    const char *name;
    
    int (*register_device)(amd_device_t *dev);
    int (*unregister_device)(amd_device_t *dev);
    
    int (*request_vram)(size_t size, uint64_t *gpu_addr);
    int (*release_vram)(uint64_t gpu_addr);
    
    int (*set_framebuffer)(uint64_t gpu_addr, uint32_t width, 
                          uint32_t height, uint32_t stride);
    
    int (*enter_power_state)(int level);
    int (*exit_power_state)(void);
} amd_platform_bridge_t;

/* Main Device Structure */
typedef struct amd_device {
    amd_gpu_device_info_t gpu_info;
    amd_gpu_handler_t *handler;
    amd_api_backend_t *backend;
    amd_platform_bridge_t *platform;
    
    /* Hardware state */
    void *hw_state;
    void *mmio_base;
    uint64_t mmio_size;
    
    /* Memory management */
    void *vram_pool;
    size_t vram_used;
    
    /* Device reference count */
    int ref_count;
    bool initialized;
    
    /* HAL integration (for real hardware) */
    OBJGPU *hal_device;  /* Bridge to real HAL layer */
    int use_hal_backend;  /* Use HAL for initialization if 1 */
} amd_device_t;

/* Public API */
int amd_device_probe(uint16_t device_id, amd_device_t **dev);
int amd_device_init(amd_device_t *dev);
int amd_device_fini(amd_device_t *dev);
void amd_device_free(amd_device_t *dev);

amd_gpu_device_info_t* amd_device_lookup(uint16_t device_id);
amd_backend_type_t amd_select_backend(amd_gpu_device_info_t *gpu);
amd_gpu_handler_t* amd_get_handler(amd_gpu_generation_t generation);
void amd_device_print_info(amd_gpu_device_info_t *gpu);

#endif /* AMD_DEVICE_H */

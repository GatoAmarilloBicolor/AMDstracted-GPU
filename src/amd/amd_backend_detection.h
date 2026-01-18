#ifndef AMD_BACKEND_DETECTION_H
#define AMD_BACKEND_DETECTION_H

#include <stdint.h>
#include <stdbool.h>

/* Backend detection and environment variable management */

typedef enum {
    AMD_ENV_BACKEND_AUTO = 0,       /* Auto-detect based on HW */
    AMD_ENV_BACKEND_RADV,           /* Force RADV */
    AMD_ENV_BACKEND_ZINK,           /* Force Zink (GL via Vulkan) */
    AMD_ENV_BACKEND_GALLIUM,        /* Force Mesa Gallium */
    AMD_ENV_BACKEND_LLVMPIPE,       /* Force software rendering */
} amd_env_backend_t;

typedef struct {
    /* Detected capabilities from hardware */
    bool hw_supports_vulkan;
    bool hw_supports_opengl;
    bool hw_supports_opencl;
    bool hw_supports_compute;
    
    /* Available on system */
    bool system_has_radv;
    bool system_has_zink;
    bool system_has_mesa_gallium;
    bool system_has_llvmpipe;
    
    /* Environment variable override */
    amd_env_backend_t env_override;
    
    /* Final selected backend */
    char selected_backend[32];
    char backend_note[128];
} amd_backend_support_t;

/* Detect backend support */
int amd_detect_backend_support(amd_backend_support_t *support);

/* Set environment variables based on support */
int amd_setup_backend_env(amd_backend_support_t *support);

/* Check if library is available */
bool amd_check_library_available(const char *libname);

/* Get environment variable overrides */
amd_env_backend_t amd_get_env_override(void);

/* Print detected capabilities */
void amd_print_backend_support(amd_backend_support_t *support);

#endif /* AMD_BACKEND_DETECTION_H */

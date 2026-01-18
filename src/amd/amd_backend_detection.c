#include "amd_backend_detection.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>

/* For setenv - BSD/Linux */
extern int setenv(const char *name, const char *value, int overwrite);

/* Detect if a library is available on the system */
bool amd_check_library_available(const char *libname)
{
    if (!libname) return false;
    
    void *handle = dlopen(libname, RTLD_LAZY | RTLD_NOLOAD);
    if (handle) {
        dlclose(handle);
        return true;
    }
    
    /* Try to load it */
    handle = dlopen(libname, RTLD_LAZY);
    if (handle) {
        dlclose(handle);
        return true;
    }
    
    return false;
}

/* Get environment variable override */
amd_env_backend_t amd_get_env_override(void)
{
    const char *env = getenv("AMD_GPU_BACKEND");
    if (!env) return AMD_ENV_BACKEND_AUTO;
    
    if (strcmp(env, "radv") == 0) return AMD_ENV_BACKEND_RADV;
    if (strcmp(env, "zink") == 0) return AMD_ENV_BACKEND_ZINK;
    if (strcmp(env, "gallium") == 0) return AMD_ENV_BACKEND_GALLIUM;
    if (strcmp(env, "llvmpipe") == 0) return AMD_ENV_BACKEND_LLVMPIPE;
    if (strcmp(env, "auto") == 0) return AMD_ENV_BACKEND_AUTO;
    
    return AMD_ENV_BACKEND_AUTO;
}

/* Detect backend support on current system */
int amd_detect_backend_support(amd_backend_support_t *support)
{
    if (!support) return -1;
    
    memset(support, 0, sizeof(amd_backend_support_t));
    
    printf("[Backend Detection]\n");
    
    /* Check for RADV (Vulkan driver) */
    printf("  Checking RADV (Vulkan)... ");
    support->system_has_radv = amd_check_library_available("libvulkan.so.1") ||
                               amd_check_library_available("libvulkan.so") ||
                               amd_check_library_available("libradv.so");
    printf("%s\n", support->system_has_radv ? "✓ available" : "✗ not found");
    if (support->system_has_radv) {
        support->hw_supports_vulkan = true;
        support->hw_supports_compute = true;
    }
    
    /* Check for Zink (GL via Vulkan) */
    printf("  Checking Zink (GL→Vulkan)... ");
    support->system_has_zink = amd_check_library_available("libzink.so");
    printf("%s\n", support->system_has_zink ? "✓ available" : "✗ not found");
    if (support->system_has_zink) {
        support->hw_supports_opengl = true;
        support->hw_supports_vulkan = true;
    }
    
    /* Check for Mesa Gallium (OpenGL) */
    printf("  Checking Mesa Gallium (OpenGL)... ");
    support->system_has_mesa_gallium = amd_check_library_available("libGL.so.1") ||
                                        amd_check_library_available("libGL.so") ||
                                        amd_check_library_available("libgallium.so");
    printf("%s\n", support->system_has_mesa_gallium ? "✓ available" : "✗ not found");
    if (support->system_has_mesa_gallium) {
        support->hw_supports_opengl = true;
    }
    
    /* Check for LLVMPipe (software) */
    printf("  Checking LLVMPipe (software)... ");
    support->system_has_llvmpipe = amd_check_library_available("libllvmpipe.so");
    printf("%s\n", support->system_has_llvmpipe ? "✓ available" : "✗ not found");
    if (support->system_has_llvmpipe) {
        support->hw_supports_opengl = true;
    }
    
    /* Check for environment override */
    support->env_override = amd_get_env_override();
    
    if (support->env_override != AMD_ENV_BACKEND_AUTO) {
        printf("  Environment override: AMD_GPU_BACKEND=%d\n", support->env_override);
    }
    
    return 0;
}

/* Setup environment variables based on detected support */
int amd_setup_backend_env(amd_backend_support_t *support)
{
    if (!support) return -1;
    
    /* Check if we're on Haiku */
    #ifdef __HAIKU__
        bool should_set_env = true;
    #else
        bool should_set_env = false;
    #endif
    
    if (should_set_env) {
        printf("\n[Setting Environment Variables]\n");
    } else {
        printf("\n[Suggested Environment Variables]\n");
        printf("  (Not setting on Linux - configure manually if needed)\n\n");
    }
    
    /* Determine backend based on support and override */
    const char *backend_choice = NULL;
    
    if (support->env_override != AMD_ENV_BACKEND_AUTO) {
        /* Use environment override */
        switch (support->env_override) {
            case AMD_ENV_BACKEND_RADV:
                if (support->system_has_radv) {
                    backend_choice = "radv";
                    snprintf(support->selected_backend, sizeof(support->selected_backend), "RADV");
                    snprintf(support->backend_note, sizeof(support->backend_note), 
                            "User override: RADV (Vulkan)");
                } else {
                    printf("  WARNING: RADV requested but not available\n");
                    backend_choice = "gallium";
                }
                break;
                
            case AMD_ENV_BACKEND_ZINK:
                if (support->system_has_zink) {
                    backend_choice = "zink";
                    snprintf(support->selected_backend, sizeof(support->selected_backend), "Zink");
                    snprintf(support->backend_note, sizeof(support->backend_note),
                            "User override: Zink (OpenGL via Vulkan)");
                } else {
                    printf("  WARNING: Zink requested but not available\n");
                    backend_choice = "gallium";
                }
                break;
                
            case AMD_ENV_BACKEND_GALLIUM:
                if (support->system_has_mesa_gallium) {
                    backend_choice = "gallium";
                    snprintf(support->selected_backend, sizeof(support->selected_backend), "Mesa Gallium");
                    snprintf(support->backend_note, sizeof(support->backend_note),
                            "User override: Mesa Gallium (OpenGL)");
                } else {
                    printf("  WARNING: Mesa Gallium requested but not available\n");
                    backend_choice = "llvmpipe";
                }
                break;
                
            case AMD_ENV_BACKEND_LLVMPIPE:
                if (support->system_has_llvmpipe) {
                    backend_choice = "llvmpipe";
                    snprintf(support->selected_backend, sizeof(support->selected_backend), "LLVMPipe");
                    snprintf(support->backend_note, sizeof(support->backend_note),
                            "User override: LLVMPipe (Software)");
                } else {
                    printf("  WARNING: LLVMPipe requested but not available\n");
                }
                break;
                
            default:
                backend_choice = "gallium";
                break;
        }
    } else {
        /* Auto-detect: prefer Vulkan for modern, fallback to OpenGL, then software */
        if (support->system_has_radv && support->hw_supports_vulkan) {
            backend_choice = "radv";
            snprintf(support->selected_backend, sizeof(support->selected_backend), "RADV");
            snprintf(support->backend_note, sizeof(support->backend_note),
                    "Auto-detected: RADV (Vulkan)");
        } else if (support->system_has_zink && support->hw_supports_opengl) {
            backend_choice = "zink";
            snprintf(support->selected_backend, sizeof(support->selected_backend), "Zink");
            snprintf(support->backend_note, sizeof(support->backend_note),
                    "Auto-detected: Zink (GL→Vulkan fallback)");
        } else if (support->system_has_mesa_gallium && support->hw_supports_opengl) {
            backend_choice = "gallium";
            snprintf(support->selected_backend, sizeof(support->selected_backend), "Mesa Gallium");
            snprintf(support->backend_note, sizeof(support->backend_note),
                    "Auto-detected: Mesa Gallium (OpenGL)");
        } else if (support->system_has_llvmpipe) {
            backend_choice = "llvmpipe";
            snprintf(support->selected_backend, sizeof(support->selected_backend), "LLVMPipe");
            snprintf(support->backend_note, sizeof(support->backend_note),
                    "Fallback: LLVMPipe (Software)");
        } else {
            /* Last resort */
            snprintf(support->selected_backend, sizeof(support->selected_backend), "None");
            snprintf(support->backend_note, sizeof(support->backend_note),
                    "ERROR: No suitable backend found");
            printf("  ERROR: No suitable graphics backend found\n");
            return -1;
        }
    }
    
    /* Set or suggest environment variables */
    if (backend_choice) {
        if (should_set_env) {
            printf("  Setting AMD_GPU_BACKEND=%s\n", backend_choice);
            setenv("AMD_GPU_BACKEND", backend_choice, 1);
        } else {
            printf("  export AMD_GPU_BACKEND=%s\n", backend_choice);
        }
    }
    
    /* Set Mesa driver if using Gallium/Zink */
    if (strcmp(backend_choice, "gallium") == 0 || strcmp(backend_choice, "zink") == 0) {
        if (should_set_env) {
            printf("  Setting LIBGL_ALWAYS_INDIRECT=1 (OpenGL mode)\n");
            setenv("LIBGL_ALWAYS_INDIRECT", "1", 1);
            
            printf("  Setting MESA_GL_VERSION_OVERRIDE (compatibility)\n");
            setenv("MESA_GL_VERSION_OVERRIDE", "4.5", 1);
        } else {
            printf("  export LIBGL_ALWAYS_INDIRECT=1\n");
            printf("  export MESA_GL_VERSION_OVERRIDE=4.5\n");
        }
    }
    
    /* Set Vulkan driver if using RADV/Zink */
    if (strcmp(backend_choice, "radv") == 0 || strcmp(backend_choice, "zink") == 0) {
        if (should_set_env) {
            printf("  Setting VK_DRIVER_FILES (Vulkan config)\n");
            setenv("VK_DRIVER_FILES", "/etc/vulkan/icd.d/amd_icd.json", 1);
        } else {
            printf("  export VK_DRIVER_FILES=/etc/vulkan/icd.d/amd_icd.json\n");
        }
    }
    
    printf("  Selected: %s\n", support->selected_backend);
    printf("  Note: %s\n", support->backend_note);
    
    return 0;
}

/* Print backend support information */
void amd_print_backend_support(amd_backend_support_t *support)
{
    if (!support) return;
    
    printf("\n=== AMD Backend Support ===\n\n");
    
    printf("Hardware Capabilities:\n");
    printf("  Vulkan:  %s\n", support->hw_supports_vulkan ? "✓" : "✗");
    printf("  OpenGL:  %s\n", support->hw_supports_opengl ? "✓" : "✗");
    printf("  Compute: %s\n", support->hw_supports_compute ? "✓" : "✗");
    printf("\n");
    
    printf("System Libraries:\n");
    printf("  RADV:        %s\n", support->system_has_radv ? "✓" : "✗");
    printf("  Zink:        %s\n", support->system_has_zink ? "✓" : "✗");
    printf("  Mesa Gallium:%s\n", support->system_has_mesa_gallium ? "✓" : "✗");
    printf("  LLVMPipe:    %s\n", support->system_has_llvmpipe ? "✓" : "✗");
    printf("\n");
    
    printf("Selected Backend: %s\n", support->selected_backend);
    printf("Reason: %s\n", support->backend_note);
    printf("\n");
}

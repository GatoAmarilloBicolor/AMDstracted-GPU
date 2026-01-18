/*
 * Engine Manager - GPU Graphics Engine Acquisition & Control
 * Handles engine ownership, synchronization, and fence tracking
 * 
 * Developed by: Haiku Imposible Team (HIT)
 */

#include "../../hal/hal.h"
#include "../../../os/os_primitives.h"
#include <string.h>

/* ============================================================================
 * Engine State Management
 * ============================================================================ */

// Maximum concurrent engines
#define MAX_GPU_ENGINES 8

typedef struct {
    uint32_t token;              // Unique engine ID
    uint32_t owner_pid;          // Process owning this engine
    bool in_use;                 // Currently acquired?
    uint64_t last_fence_value;   // Last submitted fence
    uint64_t completed_fence;    // Last completed fence
    os_prim_lock_t lock;         // Synchronization (if supported)
} engine_t;

// Global engine pool
static engine_t engines[MAX_GPU_ENGINES];
static uint32_t next_token = 0x1000;  // Start tokens at 0x1000

// Initialize engine pool
static int engine_pool_initialized = 0;

/*
 * Initialize the engine manager (call once at startup)
 */
int engine_manager_init(void) {
    if (engine_pool_initialized) {
        return 0;
    }

    os_prim_log("Engine Manager: Initializing pool of %d engines...\n", MAX_GPU_ENGINES);

    memset(engines, 0, sizeof(engines));
    
    for (int i = 0; i < MAX_GPU_ENGINES; i++) {
        engines[i].token = next_token + i;
        engines[i].owner_pid = 0;
        engines[i].in_use = false;
        engines[i].last_fence_value = 0;
        engines[i].completed_fence = 0;
        // Lock initialization would go here if os_prim supports it
    }

    engine_pool_initialized = 1;
    os_prim_log("Engine Manager: Engine pool ready ✓\n");
    
    return 0;
}

/*
 * Cleanup engine manager
 */
void engine_manager_fini(void) {
    if (!engine_pool_initialized) {
        return;
    }

    os_prim_log("Engine Manager: Shutting down...\n");
    
    // Release all acquired engines
    for (int i = 0; i < MAX_GPU_ENGINES; i++) {
        if (engines[i].in_use) {
            os_prim_log("Engine Manager: Force-releasing engine %u (token=0x%x)\n",
                       i, engines[i].token);
            engines[i].in_use = false;
            engines[i].owner_pid = 0;
        }
    }

    engine_pool_initialized = 0;
}

/*
 * Find free engine matching requested capabilities
 * Returns engine index or -1 if none available
 */
static int engine_find_free(uint32_t capabilities) {
    for (int i = 0; i < MAX_GPU_ENGINES; i++) {
        if (!engines[i].in_use) {
            // In real implementation, check if engine has requested capabilities
            // For now, all engines are generic
            return i;
        }
    }
    
    return -1;  // No free engines
}

/*
 * Acquire GPU engine for exclusive use
 * 
 * Returns:
 *   0 = success, engine_token populated
 *  -1 = no engines available
 *  -2 = timeout waiting for engine
 */
int engine_acquire(uint32_t capabilities, uint32_t max_wait_ms,
                   uint32_t *engine_token) {
    if (!engine_token) {
        return -1;
    }

    engine_manager_init();  // Ensure initialized

    os_prim_log("Engine Manager: Acquire request (caps=0x%x, wait=%ums)\n",
                capabilities, max_wait_ms);

    // Try to find free engine
    int timeout_count = (max_wait_ms + 9) / 10;  // 10ms polling granularity
    
    while (timeout_count-- > 0) {
        int engine_idx = engine_find_free(capabilities);
        
        if (engine_idx >= 0) {
            // Found free engine - acquire it
            engines[engine_idx].in_use = true;
            engines[engine_idx].owner_pid = os_prim_get_current_pid();
            engines[engine_idx].last_fence_value = 0;
            engines[engine_idx].completed_fence = 0;
            
            *engine_token = engines[engine_idx].token;
            
            os_prim_log("Engine Manager: Acquired engine %d (token=0x%x, pid=%u)\n",
                       engine_idx, *engine_token, engines[engine_idx].owner_pid);
            
            return 0;  // Success
        }
        
        // Wait before retry
        if (timeout_count > 0) {
            os_prim_delay_us(10000);  // 10ms
        }
    }

    os_prim_log("Engine Manager: No free engines after %ums wait\n", max_wait_ms);
    return -2;  // Timeout
}

/*
 * Release engine back to pool
 */
int engine_release(uint32_t engine_token) {
    os_prim_log("Engine Manager: Release request (token=0x%x)\n", engine_token);

    // Find engine by token
    for (int i = 0; i < MAX_GPU_ENGINES; i++) {
        if (engines[i].token == engine_token) {
            if (!engines[i].in_use) {
                os_prim_log("Engine Manager: WARNING - Engine not in use\n");
                return -1;
            }
            
            engines[i].in_use = false;
            engines[i].owner_pid = 0;
            
            os_prim_log("Engine Manager: Released engine %d (token=0x%x)\n", i, engine_token);
            
            return 0;
        }
    }

    os_prim_log("Engine Manager: ERROR - Invalid engine token 0x%x\n", engine_token);
    return -1;
}

/*
 * Submit work to engine and get fence value
 * Real implementation would submit to ring buffer here
 */
int engine_submit_work(uint32_t engine_token, const void *commands,
                       uint32_t command_size, uint64_t *fence_value) {
    if (!fence_value) {
        return -1;
    }

    os_prim_log("Engine Manager: Submit work (token=0x%x, size=%u bytes)\n",
                engine_token, command_size);

    // Find engine
    for (int i = 0; i < MAX_GPU_ENGINES; i++) {
        if (engines[i].token == engine_token) {
            if (!engines[i].in_use) {
                os_prim_log("Engine Manager: ERROR - Engine not acquired\n");
                return -1;
            }
            
            // Allocate next fence value
            engines[i].last_fence_value++;
            *fence_value = engines[i].last_fence_value;
            
            os_prim_log("Engine Manager: Submitted (fence=0x%llx)\n", *fence_value);
            
            // TODO: Actually submit to ring buffer here
            // For now, just track the fence
            
            return 0;
        }
    }

    return -1;
}

/*
 * Wait for fence to complete
 * Polls GPU hardware status
 */
int engine_wait_fence(uint32_t engine_token, uint64_t fence_value,
                      uint32_t timeout_ms) {
    os_prim_log("Engine Manager: Wait fence (token=0x%x, fence=0x%llx, timeout=%ums)\n",
                engine_token, fence_value, timeout_ms);

    // Find engine
    engine_t *eng = NULL;
    for (int i = 0; i < MAX_GPU_ENGINES; i++) {
        if (engines[i].token == engine_token) {
            eng = &engines[i];
            break;
        }
    }

    if (!eng) {
        os_prim_log("Engine Manager: ERROR - Invalid token\n");
        return -1;
    }

    if (!eng->in_use) {
        os_prim_log("Engine Manager: ERROR - Engine not acquired\n");
        return -1;
    }

    // Poll for completion
    int timeout_count = (timeout_ms + 9) / 10;
    
    while (timeout_count-- > 0) {
        // In real implementation, read GPU fence register here
        // For simulation, just increment completed fence
        eng->completed_fence = eng->last_fence_value;
        
        if (eng->completed_fence >= fence_value) {
            os_prim_log("Engine Manager: Fence completed ✓\n");
            return 0;
        }
        
        if (timeout_count > 0) {
            os_prim_delay_us(10000);  // 10ms polling
        }
    }

    os_prim_log("Engine Manager: Fence timeout\n");
    return -1;
}

/*
 * Get engine status (for debugging)
 */
typedef struct {
    uint32_t token;
    bool in_use;
    uint32_t owner_pid;
    uint64_t last_fence;
    uint64_t completed_fence;
} engine_status_t;

int engine_get_status(uint32_t engine_token, engine_status_t *status) {
    if (!status) {
        return -1;
    }

    for (int i = 0; i < MAX_GPU_ENGINES; i++) {
        if (engines[i].token == engine_token) {
            status->token = engines[i].token;
            status->in_use = engines[i].in_use;
            status->owner_pid = engines[i].owner_pid;
            status->last_fence = engines[i].last_fence_value;
            status->completed_fence = engines[i].completed_fence;
            return 0;
        }
    }

    return -1;
}

/*
 * Dump all engine status (debugging)
 */
void engine_dump_status(void) {
    os_prim_log("\n=== Engine Manager Status ===\n");
    
    for (int i = 0; i < MAX_GPU_ENGINES; i++) {
        os_prim_log("Engine %d: token=0x%x, in_use=%d, pid=%u, fences=0x%llx/0x%llx\n",
                   i, engines[i].token, engines[i].in_use, engines[i].owner_pid,
                   engines[i].last_fence_value, engines[i].completed_fence);
    }
    
    os_prim_log("============================\n\n");
}

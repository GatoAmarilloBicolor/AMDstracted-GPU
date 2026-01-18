#ifndef AMD_ENGINE_MANAGER_H
#define AMD_ENGINE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

/*
 * GPU Engine Manager - Handles acquisition, synchronization, and fence tracking
 * 
 * Usage:
 *   1. engine_manager_init() - Initialize at startup
 *   2. engine_acquire() - Get exclusive engine access
 *   3. engine_submit_work() - Submit commands and get fence
 *   4. engine_wait_fence() - Wait for completion
 *   5. engine_release() - Return engine to pool
 *   6. engine_manager_fini() - Cleanup at shutdown
 */

// Engine capabilities (bitmask)
#define ENGINE_CAP_2D      (1 << 0)   // 2D acceleration
#define ENGINE_CAP_3D      (1 << 1)   // 3D acceleration
#define ENGINE_CAP_COMPUTE (1 << 2)   // Compute
#define ENGINE_CAP_ANY     (ENGINE_CAP_2D | ENGINE_CAP_3D | ENGINE_CAP_COMPUTE)

// Initialize engine pool
int engine_manager_init(void);

// Cleanup engine pool
void engine_manager_fini(void);

// Acquire exclusive GPU engine
// Returns:
//   0 = success
//  -1 = invalid parameters
//  -2 = timeout (no engines available)
int engine_acquire(uint32_t capabilities, uint32_t max_wait_ms,
                  uint32_t *engine_token);

// Release acquired engine
// Returns:
//   0 = success
//  -1 = error (invalid token or not acquired)
int engine_release(uint32_t engine_token);

// Submit work to engine
// Returns:
//   0 = success, fence_value contains completion fence
//  -1 = error
int engine_submit_work(uint32_t engine_token, const void *commands,
                      uint32_t command_size, uint64_t *fence_value);

// Wait for fence completion
// Returns:
//   0 = fence completed
//  -1 = error or timeout
int engine_wait_fence(uint32_t engine_token, uint64_t fence_value,
                     uint32_t timeout_ms);

// Engine status structure
typedef struct {
    uint32_t token;
    bool in_use;
    uint32_t owner_pid;
    uint64_t last_fence;
    uint64_t completed_fence;
} engine_status_t;

// Get status of specific engine
int engine_get_status(uint32_t engine_token, engine_status_t *status);

// Dump all engine status (debugging)
void engine_dump_status(void);

#endif

/*
 * GPU Command Ring Buffer Management
 * Circular queue for GPU command submission
 * 
 * Developed by: Haiku Imposible Team (HIT)
 */

#include "../../hal/hal.h"
#include "../../../os/os_primitives.h"
#include <string.h>

/* ============================================================================
 * Ring Buffer Structure
 * ============================================================================ */

typedef struct {
    uint64_t gpu_addr;           // GPU-visible address
    void *cpu_addr;              // CPU-visible pointer
    uint32_t size_dwords;        // Ring size in dwords (32-bit words)
    uint32_t write_ptr;          // Write pointer (CPU writes here)
    uint32_t read_ptr;           // Read pointer (GPU reads here)
    uint64_t fence_value;        // Current fence value
    bool enabled;
} ring_buffer_t;

// Global ring buffers (one per type)
static ring_buffer_t gfx_ring = {0};
static ring_buffer_t dma_ring = {0};

/* ============================================================================
 * Ring Buffer Initialization
 * ============================================================================ */

/*
 * Initialize a ring buffer
 * Size is typically 256KB to 1MB
 */
int ring_buffer_init(ring_buffer_t *ring, uint32_t size_kb) {
    if (!ring || size_kb < 64 || size_kb > 4096) {
        os_prim_log("Ring: ERROR - Invalid size %uKB (64-4096 allowed)\n", size_kb);
        return -1;
    }

    uint32_t size_bytes = size_kb * 1024;
    uint32_t size_dwords = size_bytes / 4;

    os_prim_log("Ring: Initializing ring buffer (%uKB = %u dwords)...\n",
                size_kb, size_dwords);

    // Allocate ring buffer (CPU-visible)
    ring->cpu_addr = os_prim_alloc(size_bytes);
    if (!ring->cpu_addr) {
        os_prim_log("Ring: ERROR - Failed to allocate %uKB\n", size_kb);
        return -1;
    }

    // Initialize
    memset(ring->cpu_addr, 0, size_bytes);
    ring->gpu_addr = (uintptr_t)ring->cpu_addr;  // In userland, same address
    ring->size_dwords = size_dwords;
    ring->write_ptr = 0;
    ring->read_ptr = 0;
    ring->fence_value = 0;
    ring->enabled = true;

    os_prim_log("Ring: Ring buffer ready at 0x%llx (%u dwords)\n",
                ring->gpu_addr, ring->size_dwords);

    return 0;
}

/*
 * Write command(s) to ring buffer
 * Returns number of dwords written or -1 on error
 */
int ring_buffer_write(ring_buffer_t *ring, const uint32_t *commands,
                     uint32_t num_dwords) {
    if (!ring || !commands || num_dwords == 0) {
        return -1;
    }

    if (num_dwords > ring->size_dwords) {
        os_prim_log("Ring: ERROR - Command too large (%u > %u dwords)\n",
                   num_dwords, ring->size_dwords);
        return -1;
    }

    uint32_t write_ptr = ring->write_ptr;
    uint32_t available = ring->size_dwords - write_ptr;

    // Check if we have space
    if (num_dwords > available) {
        // Need to wrap around
        // For now, fail if we can't fit
        if (write_ptr + num_dwords > ring->size_dwords) {
            os_prim_log("Ring: WARNING - Ring full, wrapping to start\n");
            write_ptr = 0;
            available = ring->size_dwords;
        }
    }

    // Write commands to ring
    uint32_t *ring_ptr = (uint32_t *)ring->cpu_addr + write_ptr;
    
    for (uint32_t i = 0; i < num_dwords; i++) {
        ring_ptr[i] = commands[i];
    }

    // Update write pointer
    ring->write_ptr = (write_ptr + num_dwords) % ring->size_dwords;

    os_prim_log("Ring: Wrote %u dwords (ptr: %u → %u)\n",
                num_dwords, write_ptr, ring->write_ptr);

    return num_dwords;
}

/*
 * Get current write pointer
 */
uint32_t ring_buffer_get_write_ptr(ring_buffer_t *ring) {
    if (!ring) return 0;
    return ring->write_ptr;
}

/*
 * Get current read pointer (GPU progress)
 */
uint32_t ring_buffer_get_read_ptr(ring_buffer_t *ring) {
    if (!ring) return 0;
    return ring->read_ptr;
}

/*
 * Check if ring is empty
 */
bool ring_buffer_is_empty(ring_buffer_t *ring) {
    if (!ring) return true;
    return ring->write_ptr == ring->read_ptr;
}

/*
 * Check free space in ring
 */
uint32_t ring_buffer_get_free_space(ring_buffer_t *ring) {
    if (!ring) return 0;
    
    if (ring->write_ptr >= ring->read_ptr) {
        return ring->size_dwords - (ring->write_ptr - ring->read_ptr);
    } else {
        return ring->read_ptr - ring->write_ptr;
    }
}

/*
 * Allocate fence value
 */
uint64_t ring_buffer_alloc_fence(ring_buffer_t *ring) {
    if (!ring) return 0;
    return ++ring->fence_value;
}

/*
 * Ring buffer reset (careful - only when GPU is idle)
 */
void ring_buffer_reset(ring_buffer_t *ring) {
    if (!ring) return;
    
    os_prim_log("Ring: Resetting ring buffer\n");
    memset(ring->cpu_addr, 0, ring->size_dwords * 4);
    ring->write_ptr = 0;
    ring->read_ptr = 0;
    ring->fence_value = 0;
}

/*
 * Cleanup ring buffer
 */
void ring_buffer_fini(ring_buffer_t *ring) {
    if (!ring || !ring->cpu_addr) return;
    
    os_prim_log("Ring: Freeing ring buffer\n");
    os_prim_free(ring->cpu_addr);
    ring->cpu_addr = NULL;
    ring->gpu_addr = 0;
    ring->enabled = false;
}

/* ============================================================================
 * Global Ring Management
 * ============================================================================ */

/*
 * Initialize all rings
 */
int ring_manager_init(void) {
    os_prim_log("Ring Manager: Initializing all rings...\n");

    // Graphics ring (256KB typical)
    if (ring_buffer_init(&gfx_ring, 256) != 0) {
        os_prim_log("Ring Manager: ERROR - Failed to init GFX ring\n");
        return -1;
    }

    // DMA ring (64KB typical)
    if (ring_buffer_init(&dma_ring, 64) != 0) {
        os_prim_log("Ring Manager: ERROR - Failed to init DMA ring\n");
        ring_buffer_fini(&gfx_ring);
        return -1;
    }

    os_prim_log("Ring Manager: All rings initialized ✓\n");
    return 0;
}

/*
 * Get GFX ring
 */
ring_buffer_t *ring_get_gfx(void) {
    return &gfx_ring;
}

/*
 * Get DMA ring
 */
ring_buffer_t *ring_get_dma(void) {
    return &dma_ring;
}

/*
 * Submit commands and get fence
 */
int ring_submit_commands(ring_buffer_t *ring, const uint32_t *commands,
                        uint32_t num_dwords, uint64_t *fence_out) {
    if (!ring || !commands || !fence_out) {
        return -1;
    }

    // Check space
    uint32_t free_space = ring_buffer_get_free_space(ring);
    if (num_dwords > free_space) {
        os_prim_log("Ring: Not enough space (need %u, have %u)\n",
                   num_dwords, free_space);
        return -1;
    }

    // Allocate fence before writing
    *fence_out = ring_buffer_alloc_fence(ring);

    // Write commands
    int ret = ring_buffer_write(ring, commands, num_dwords);
    if (ret < 0) {
        return -1;
    }

    os_prim_log("Ring: Submitted %u dwords (fence=0x%llx)\n",
                num_dwords, *fence_out);

    return 0;
}

/*
 * Ring status (debugging)
 */
void ring_dump_status(ring_buffer_t *ring) {
    if (!ring) return;

    os_prim_log("\n=== Ring Buffer Status ===\n");
    os_prim_log("GPU Address: 0x%llx\n", ring->gpu_addr);
    os_prim_log("Size: %u dwords (%uKB)\n", ring->size_dwords, 
                ring->size_dwords * 4 / 1024);
    os_prim_log("Write Ptr: %u\n", ring->write_ptr);
    os_prim_log("Read Ptr: %u\n", ring->read_ptr);
    os_prim_log("Free Space: %u dwords\n", ring_buffer_get_free_space(ring));
    os_prim_log("Current Fence: 0x%llx\n", ring->fence_value);
    os_prim_log("Empty: %s\n", ring_buffer_is_empty(ring) ? "Yes" : "No");
    os_prim_log("===========================\n\n");
}

/*
 * Cleanup all rings
 */
void ring_manager_fini(void) {
    os_prim_log("Ring Manager: Shutting down...\n");
    ring_buffer_fini(&gfx_ring);
    ring_buffer_fini(&dma_ring);
}

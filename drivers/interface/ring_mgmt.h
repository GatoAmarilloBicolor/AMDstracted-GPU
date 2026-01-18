// Ring Management - Hardware Command Ring Interface

#ifndef RING_MGMT_H
#define RING_MGMT_H

#include <stdint.h>
#include <stddef.h>

// Ring types
#define RING_TYPE_GFX 0
#define RING_TYPE_COMPUTE 1
#define RING_TYPE_SDMA 2

// Ring structure
typedef struct {
    uint32_t ring_id;
    uint32_t ring_type;
    uintptr_t ring_base;    // MMIO base for ring registers
    void *ring_buffer;      // CPU mapping of ring buffer
    uint64_t ring_gpu_addr; // GPU VA of ring buffer
    uint32_t ring_size;     // Ring buffer size in dwords
    uint32_t wptr;          // Write pointer
    uint32_t rptr;          // Read pointer
} gpu_ring_t;

// Initialize ring
int ring_init(gpu_ring_t *ring, uint32_t ring_id, uint32_t ring_type,
              uintptr_t mmio_base, uint64_t gpu_addr, uint32_t size);

// Cleanup ring
void ring_fini(gpu_ring_t *ring);

// Submit commands to ring
int ring_submit_commands(gpu_ring_t *ring, const uint32_t *cmds, uint32_t num_cmds);

// Wait for ring idle
int ring_wait_idle(gpu_ring_t *ring, uint32_t timeout_us);

// Get ring status
uint32_t ring_get_wptr(gpu_ring_t *ring);
uint32_t ring_get_rptr(gpu_ring_t *ring);

#endif // RING_MGMT_H
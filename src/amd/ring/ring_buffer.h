#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

/*
 * GPU Command Ring Buffer - Circular queue for GPU submissions
 * 
 * Ring buffers are the standard way to submit work to modern GPUs:
 * - CPU writes commands to ring memory
 * - CPU updates write pointer
 * - GPU reads from ring, updating read pointer
 * - When write_ptr catches read_ptr, ring is full
 */

typedef struct {
    uint64_t gpu_addr;
    void *cpu_addr;
    uint32_t size_dwords;
    uint32_t write_ptr;
    uint32_t read_ptr;
    uint64_t fence_value;
    bool enabled;
} ring_buffer_t;

// Ring buffer operations
int ring_buffer_init(ring_buffer_t *ring, uint32_t size_kb);
void ring_buffer_fini(ring_buffer_t *ring);

int ring_buffer_write(ring_buffer_t *ring, const uint32_t *commands,
                     uint32_t num_dwords);

uint32_t ring_buffer_get_write_ptr(ring_buffer_t *ring);
uint32_t ring_buffer_get_read_ptr(ring_buffer_t *ring);
bool ring_buffer_is_empty(ring_buffer_t *ring);
uint32_t ring_buffer_get_free_space(ring_buffer_t *ring);

uint64_t ring_buffer_alloc_fence(ring_buffer_t *ring);
void ring_buffer_reset(ring_buffer_t *ring);

void ring_dump_status(ring_buffer_t *ring);

// Ring manager (global rings)
int ring_manager_init(void);
void ring_manager_fini(void);

ring_buffer_t *ring_get_gfx(void);
ring_buffer_t *ring_get_dma(void);

int ring_submit_commands(ring_buffer_t *ring, const uint32_t *commands,
                        uint32_t num_dwords, uint64_t *fence_out);

#endif

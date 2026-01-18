/*
 * 2D Graphics Operations - Blit and Fill
 * Hardware-accelerated rectangle copy and fill operations
 * 
 * Developed by: Haiku Imposible Team (HIT)
 */

#include "../hal/hal.h"
#include "../../os/os_primitives.h"
#include <string.h>

/* ============================================================================
 * 2D Command Packet Structures
 * ============================================================================ */

// RDNA 2D packet format (simplified)
typedef struct {
    uint32_t header;              // Packet type and size
    uint32_t src_base_addr;       // Source surface address
    uint32_t dst_base_addr;       // Dest surface address
    uint32_t pitch;               // Pitch in pixels
    uint32_t src_x;               // Source X coordinate
    uint32_t src_y;               // Source Y coordinate
    uint32_t dst_x;               // Dest X coordinate
    uint32_t dst_y;               // Dest Y coordinate
    uint32_t width;               // Width in pixels
    uint32_t height;              // Height in pixels
    uint32_t rop;                 // Raster operation (0xCC = copy)
} gfx_2d_blit_t;

// Fill command
typedef struct {
    uint32_t header;              // Packet type and size
    uint32_t dst_base_addr;       // Dest surface address
    uint32_t pitch;               // Pitch in pixels
    uint32_t dst_x;               // Dest X coordinate
    uint32_t dst_y;               // Dest Y coordinate
    uint32_t width;               // Width in pixels
    uint32_t height;              // Height in pixels
    uint32_t fill_color;          // Color to fill (32-bit RGBA)
} gfx_2d_fill_t;

/* ============================================================================
 * 2D Operations Implementation
 * ============================================================================ */

/*
 * Build a 2D blit packet (copy rectangle)
 */
int gfx_2d_build_blit(gfx_2d_blit_t *packet,
                     uint64_t src_addr, uint64_t dst_addr,
                     uint32_t pitch,
                     uint32_t src_x, uint32_t src_y,
                     uint32_t dst_x, uint32_t dst_y,
                     uint32_t width, uint32_t height) {
    if (!packet) {
        return -1;
    }

    memset(packet, 0, sizeof(gfx_2d_blit_t));

    // Header: type=BLIT, size=10 dwords
    packet->header = (1 << 30) | 10;  // Type=1 (blit), size=10
    
    packet->src_base_addr = (uint32_t)(src_addr & 0xFFFFFFFF);
    packet->dst_base_addr = (uint32_t)(dst_addr & 0xFFFFFFFF);
    packet->pitch = pitch;
    packet->src_x = src_x;
    packet->src_y = src_y;
    packet->dst_x = dst_x;
    packet->dst_y = dst_y;
    packet->width = width;
    packet->height = height;
    packet->rop = 0xCC;  // ROP_COPY

    os_prim_log("2D: Built BLIT packet: src=0x%llx dst=0x%llx %ux%u @ (%u,%u) -> (%u,%u)\n",
                src_addr, dst_addr, width, height, src_x, src_y, dst_x, dst_y);

    return 0;
}

/*
 * Build a 2D fill packet (rectangle fill)
 */
int gfx_2d_build_fill(gfx_2d_fill_t *packet,
                     uint64_t dst_addr,
                     uint32_t pitch,
                     uint32_t dst_x, uint32_t dst_y,
                     uint32_t width, uint32_t height,
                     uint32_t color) {
    if (!packet) {
        return -1;
    }

    memset(packet, 0, sizeof(gfx_2d_fill_t));

    // Header: type=FILL, size=8 dwords
    packet->header = (2 << 30) | 8;  // Type=2 (fill), size=8
    
    packet->dst_base_addr = (uint32_t)(dst_addr & 0xFFFFFFFF);
    packet->pitch = pitch;
    packet->dst_x = dst_x;
    packet->dst_y = dst_y;
    packet->width = width;
    packet->height = height;
    packet->fill_color = color;

    os_prim_log("2D: Built FILL packet: dst=0x%llx color=0x%08x %ux%u @ (%u,%u)\n",
                dst_addr, color, width, height, dst_x, dst_y);

    return 0;
}

/*
 * Validate 2D blit parameters
 */
int gfx_2d_validate_blit(uint32_t width, uint32_t height,
                        uint32_t src_pitch, uint32_t dst_pitch,
                        uint32_t src_x, uint32_t src_y,
                        uint32_t dst_x, uint32_t dst_y) {
    // Width and height must be non-zero and reasonable
    if (width == 0 || height == 0) {
        os_prim_log("2D: ERROR - Invalid blit dimensions: %ux%u\n", width, height);
        return -1;
    }

    if (width > 8192 || height > 8192) {
        os_prim_log("2D: ERROR - Blit too large: %ux%u (max 8192x8192)\n", width, height);
        return -1;
    }

    // Check coordinates don't overflow
    if ((uint32_t)src_x + width > src_pitch) {
        os_prim_log("2D: ERROR - Source region exceeds pitch\n");
        return -1;
    }

    if ((uint32_t)dst_x + width > dst_pitch) {
        os_prim_log("2D: ERROR - Dest region exceeds pitch\n");
        return -1;
    }

    return 0;
}

/*
 * Validate 2D fill parameters
 */
int gfx_2d_validate_fill(uint32_t width, uint32_t height,
                        uint32_t pitch,
                        uint32_t dst_x, uint32_t dst_y) {
    // Width and height must be non-zero
    if (width == 0 || height == 0) {
        os_prim_log("2D: ERROR - Invalid fill dimensions: %ux%u\n", width, height);
        return -1;
    }

    if (width > 8192 || height > 8192) {
        os_prim_log("2D: ERROR - Fill too large: %ux%u (max 8192x8192)\n", width, height);
        return -1;
    }

    // Check coordinates don't overflow
    if ((uint32_t)dst_x + width > pitch) {
        os_prim_log("2D: ERROR - Fill region exceeds pitch\n");
        return -1;
    }

    return 0;
}

/*
 * Get 2D packet size (in bytes)
 */
uint32_t gfx_2d_get_packet_size(uint32_t header) {
    uint32_t dwords = header & 0x3FFF;
    return dwords * 4;  // Convert dwords to bytes
}

/*
 * Check if 2D operation completed
 */
bool gfx_2d_is_idle(struct OBJGPU *adev) {
    // In real implementation, read 2D engine status from GPU
    // For now, always report idle
    return true;
}

/*
 * Wait for 2D engine to finish
 */
int gfx_2d_wait_idle(struct OBJGPU *adev, uint32_t timeout_ms) {
    // Simple timeout loop
    int count = timeout_ms / 10;
    
    while (count-- > 0) {
        if (gfx_2d_is_idle(adev)) {
            return 0;
        }
        os_prim_delay_us(10000);  // 10ms
    }

    os_prim_log("2D: Wait idle timeout\n");
    return -1;
}

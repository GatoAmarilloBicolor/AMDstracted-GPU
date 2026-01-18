#ifndef GFX_2D_H
#define GFX_2D_H

#include <stdint.h>
#include <stdbool.h>

/*
 * 2D Graphics Acceleration API
 * 
 * Supports hardware-accelerated:
 * - Rectangle copy (BLIT)
 * - Rectangle fill (FILL)
 */

// 2D Command Packet Types
#define GFX_2D_PACKET_BLIT   1
#define GFX_2D_PACKET_FILL   2

// Raster Operations
#define GFX_2D_ROP_COPY      0xCC  // SRC
#define GFX_2D_ROP_XOR       0x66  // SRC XOR DST
#define GFX_2D_ROP_CLEAR     0x00  // BLACK
#define GFX_2D_ROP_SET       0xFF  // WHITE

// Blit command structure
typedef struct {
    uint32_t header;
    uint32_t src_base_addr;
    uint32_t dst_base_addr;
    uint32_t pitch;
    uint32_t src_x;
    uint32_t src_y;
    uint32_t dst_x;
    uint32_t dst_y;
    uint32_t width;
    uint32_t height;
    uint32_t rop;
} gfx_2d_blit_t;

// Fill command structure
typedef struct {
    uint32_t header;
    uint32_t dst_base_addr;
    uint32_t pitch;
    uint32_t dst_x;
    uint32_t dst_y;
    uint32_t width;
    uint32_t height;
    uint32_t fill_color;
} gfx_2d_fill_t;

// Build 2D commands
int gfx_2d_build_blit(gfx_2d_blit_t *packet,
                     uint64_t src_addr, uint64_t dst_addr,
                     uint32_t pitch,
                     uint32_t src_x, uint32_t src_y,
                     uint32_t dst_x, uint32_t dst_y,
                     uint32_t width, uint32_t height);

int gfx_2d_build_fill(gfx_2d_fill_t *packet,
                     uint64_t dst_addr,
                     uint32_t pitch,
                     uint32_t dst_x, uint32_t dst_y,
                     uint32_t width, uint32_t height,
                     uint32_t color);

// Validation
int gfx_2d_validate_blit(uint32_t width, uint32_t height,
                        uint32_t src_pitch, uint32_t dst_pitch,
                        uint32_t src_x, uint32_t src_y,
                        uint32_t dst_x, uint32_t dst_y);

int gfx_2d_validate_fill(uint32_t width, uint32_t height,
                        uint32_t pitch,
                        uint32_t dst_x, uint32_t dst_y);

// Utility
uint32_t gfx_2d_get_packet_size(uint32_t header);

// Engine status
struct OBJGPU;
bool gfx_2d_is_idle(struct OBJGPU *adev);
int gfx_2d_wait_idle(struct OBJGPU *adev, uint32_t timeout_ms);

#endif

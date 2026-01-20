/*
 * Platform-agnostic Accelerant API
 * Provides a unified interface for graphics acceleration across OSes
 * 
 * This header defines abstract types and functions that can be implemented
 * on Haiku, Linux, FreeBSD, etc.
 */

#ifndef AMD_ACCELERANT_API_H
#define AMD_ACCELERANT_API_H

#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * Status/Error codes (platform-agnostic)
 * ============================================================================ */

typedef int amd_status_t;

#define AMD_OK                      0
#define AMD_ERROR                  -1
#define AMD_ERROR_BAD_VALUE        -2
#define AMD_ERROR_NO_MEMORY        -3
#define AMD_ERROR_NOT_INITIALIZED  -4
#define AMD_ERROR_TIMEOUT          -5
#define AMD_ERROR_BUSY             -6

/* ============================================================================
 * Display Configuration Types (platform-agnostic)
 * ============================================================================ */

typedef struct {
    uint16_t width;
    uint16_t height;
    float refresh;
    uint32_t flags;
} amd_display_mode_t;

typedef struct {
    uint32_t space;      /* Color space ID */
    uint32_t base;       /* Physical frame buffer base */
    uint32_t size;       /* Frame buffer size in bytes */
} amd_frame_buffer_config_t;

/* ============================================================================
 * GPU Acceleration Types (platform-agnostic)
 * ============================================================================ */

typedef struct {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} amd_rect_t;

typedef amd_rect_t amd_fill_rect_params_t;

typedef struct {
    int32_t src_left;
    int32_t src_top;
    int32_t dest_left;
    int32_t dest_top;
    int32_t width;
    int32_t height;
} amd_blit_params_t;

typedef struct {
    int32_t src_left;
    int32_t src_top;
    int32_t dest_left;
    int32_t dest_top;
    int32_t width;
    int32_t height;
    uint32_t transparent_color;
} amd_transparent_blit_params_t;

typedef struct {
    int32_t src_left;
    int32_t src_top;
    int32_t src_width;
    int32_t src_height;
    int32_t dest_left;
    int32_t dest_top;
    int32_t dest_width;
    int32_t dest_height;
} amd_scaled_blit_params_t;

/* Opaque GPU engine and synchronization tokens */
typedef void* amd_engine_token_t;
typedef void* amd_sync_token_t;

/* ============================================================================
 * Device Information
 * ============================================================================ */

typedef struct {
    uint32_t version;
    uint32_t dac_version;
    uint32_t ram;           /* VRAM in MB */
    uint32_t tmds_version;
    uint32_t chipset_id;
    uint32_t revision_id;
} amd_device_info_t;

/* ============================================================================
 * Accelerant Callbacks (function pointers)
 * ============================================================================ */

typedef amd_status_t (*amd_init_fn)(int fd);
typedef void (*amd_uninit_fn)(void);
typedef amd_status_t (*amd_get_device_info_fn)(amd_device_info_t *info);
typedef uint32_t (*amd_mode_count_fn)(void);
typedef amd_status_t (*amd_get_mode_list_fn)(amd_display_mode_t *modes);
typedef amd_status_t (*amd_set_display_mode_fn)(amd_display_mode_t *mode);
typedef amd_status_t (*amd_get_display_mode_fn)(amd_display_mode_t *mode);
typedef amd_status_t (*amd_get_pixel_clock_limits_fn)(amd_display_mode_t *mode,
                                                      uint32_t *min_clock,
                                                      uint32_t *max_clock);
typedef amd_status_t (*amd_get_frame_buffer_config_fn)(amd_frame_buffer_config_t *config);
typedef uint32_t (*amd_engine_count_fn)(void);
typedef amd_status_t (*amd_acquire_engine_fn)(uint32_t engine,
                                              uint32_t caps,
                                              amd_sync_token_t *sync,
                                              amd_engine_token_t *engine_token);
typedef amd_status_t (*amd_release_engine_fn)(amd_engine_token_t engine_token,
                                              amd_sync_token_t *sync);
typedef void (*amd_fill_rectangle_fn)(amd_engine_token_t engine,
                                      uint32_t color,
                                      amd_fill_rect_params_t *list,
                                      uint32_t count);
typedef void (*amd_invert_rectangle_fn)(amd_engine_token_t engine,
                                        amd_fill_rect_params_t *list,
                                        uint32_t count);
typedef void (*amd_blit_fn)(amd_engine_token_t engine,
                            amd_blit_params_t *list,
                            uint32_t count);
typedef void (*amd_transparent_blit_fn)(amd_engine_token_t engine,
                                        amd_transparent_blit_params_t *list,
                                        uint32_t count);
typedef void (*amd_scale_blit_fn)(amd_engine_token_t engine,
                                  amd_scaled_blit_params_t *list,
                                  uint32_t count);
typedef amd_status_t (*amd_wait_engine_idle_fn)(void);
typedef void (*amd_move_cursor_fn)(uint16_t x, uint16_t y);
typedef void (*amd_show_cursor_fn)(bool is_visible);
typedef amd_status_t (*amd_set_cursor_shape_fn)(uint16_t width,
                                                uint16_t height,
                                                uint16_t hot_x,
                                                uint16_t hot_y,
                                                const uint8_t *and_mask,
                                                const uint8_t *xor_mask);

/* ============================================================================
 * Platform-specific hook function
 * Implemented per-OS to return appropriate function pointers
 * ============================================================================ */

void* amd_get_accelerant_hook(uint32_t feature, void *data);

#endif /* AMD_ACCELERANT_API_H */

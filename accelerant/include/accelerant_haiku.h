/*
 * Haiku OS Accelerant Adapter
 * Maps platform-agnostic accelerant types to Haiku-specific types
 * Works on both Haiku and non-Haiku systems
 */

#ifndef AMD_ACCELERANT_HAIKU_H
#define AMD_ACCELERANT_HAIKU_H

#include "accelerant_api.h"

/* Haiku accelerant types - defined universally */
/* These match Haiku's accelerant.h interface */

typedef int status_t;

typedef struct {
    uint16_t width;
    uint16_t height;
    float refresh;
    uint32_t flags;
} display_mode;

typedef struct {
    uint32_t space;
    uint32_t base;
    uint32_t size;
} frame_buffer_config;

typedef struct {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} fill_rect_params;

typedef struct {
    int32_t src_left;
    int32_t src_top;
    int32_t dest_left;
    int32_t dest_top;
    int32_t width;
    int32_t height;
} blit_params;

typedef struct {
    int32_t src_left;
    int32_t src_top;
    int32_t dest_left;
    int32_t dest_top;
    int32_t width;
    int32_t height;
    uint32_t transparent_color;
} transparent_blit_params;

typedef struct {
    int32_t src_left;
    int32_t src_top;
    int32_t src_width;
    int32_t src_height;
    int32_t dest_left;
    int32_t dest_top;
    int32_t dest_width;
    int32_t dest_height;
} scaled_blit_params;

typedef struct {
    uint32_t magic;
} engine_token;

typedef struct {
    uint32_t magic;
} sync_token;

typedef struct {
    uint32_t version;
    uint32_t dac_version;
    uint32_t ram;
    uint32_t tmds_version;
} accelerant_device_info;

/* Haiku accelerant hook constants */
#define B_INIT_ACCELERANT              0x08000000
#define B_UNINIT_ACCELERANT            0x08000001
#define B_GET_ACCELERANT_DEVICE_INFO   0x08000002
#define B_ACCELERANT_MODE_COUNT        0x08000003
#define B_GET_MODE_LIST                0x08000004
#define B_SET_DISPLAY_MODE             0x08000005
#define B_GET_DISPLAY_MODE             0x08000006
#define B_GET_PIXEL_CLOCK_LIMITS       0x08000007
#define B_GET_FRAME_BUFFER_CONFIG      0x08000008
#define B_ACCELERANT_ENGINE_COUNT      0x08000009
#define B_ACQUIRE_ENGINE               0x0800000A
#define B_RELEASE_ENGINE               0x0800000B
#define B_WAIT_ENGINE_IDLE             0x0800000C
#define B_FILL_RECTANGLE               0x0800000D
#define B_INVERT_RECTANGLE             0x0800000E
#define B_BLIT                         0x0800000F
#define B_TRANSPARENT_BLIT             0x08000010
#define B_SCALE_BLIT                   0x08000011
#define B_MOVE_CURSOR                  0x08000012
#define B_SHOW_CURSOR                  0x08000013
#define B_SET_CURSOR_SHAPE             0x08000014

/* Status codes */
#define B_OK                    0
#define B_ERROR                -1
#define B_BAD_VALUE            -2
#define B_NO_MEMORY            -3
#define B_BUSY                 -6
#define B_TIMED_OUT            -7

/* Type aliases - map Haiku types to themselves */
typedef display_mode amd_display_mode_haiku_t;
typedef frame_buffer_config amd_frame_buffer_config_haiku_t;
typedef fill_rect_params amd_fill_rect_params_haiku_t;
typedef blit_params amd_blit_params_haiku_t;
typedef transparent_blit_params amd_transparent_blit_params_haiku_t;
typedef scaled_blit_params amd_scaled_blit_params_haiku_t;
typedef engine_token* amd_engine_token_haiku_t;
typedef sync_token* amd_sync_token_haiku_t;

/* Conversion functions (zero overhead) */
static inline amd_display_mode_haiku_t
amd_to_haiku_display_mode(const amd_display_mode_t *mode)
{
    amd_display_mode_haiku_t hmode = {
        mode->width,
        mode->height,
        mode->refresh,
        mode->flags
    };
    return hmode;
}

static inline amd_display_mode_t
amd_from_haiku_display_mode(const amd_display_mode_haiku_t *hmode)
{
    amd_display_mode_t mode = {
        hmode->width,
        hmode->height,
        hmode->refresh,
        hmode->flags
    };
    return mode;
}

static inline amd_frame_buffer_config_haiku_t
amd_to_haiku_frame_buffer_config(const amd_frame_buffer_config_t *config)
{
    amd_frame_buffer_config_haiku_t hconfig = {
        config->space,
        config->base,
        config->size
    };
    return hconfig;
}

static inline amd_frame_buffer_config_t
amd_from_haiku_frame_buffer_config(const amd_frame_buffer_config_haiku_t *hconfig)
{
    amd_frame_buffer_config_t config = {
        hconfig->space,
        hconfig->base,
        hconfig->size
    };
    return config;
}

static inline int
amd_status_to_haiku(amd_status_t status)
{
    return status;
}

static inline amd_status_t
amd_status_from_haiku(int haiku_status)
{
    return haiku_status;
}

#endif /* AMD_ACCELERANT_HAIKU_H */

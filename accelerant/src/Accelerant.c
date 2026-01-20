/*
 * Haiku Accelerant for AMD Graphics
 * Bridges Haiku Graphics Layer with AMDGPU_Abstracted RMAPI
 * 
 * Pattern adapted from haiku-nvidia/accelerant/Accelerant.cpp
 * Converted to C and specialized for AMD RMAPI architecture
 * 
 * Copyright (c) 2024-2026 AMDGPU_Abstracted Project
 */

#include <kernel/image.h>
#include <kernel/OS.h>
#include <driver_settings.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/* ============================================================================
 * Type Definitions (Adapted from NVIDIA pattern)
 * ============================================================================ */

/* Accelerant signature for device matching */
#define AMD_ACCELERANT_SIGNATURE 0x414D4447  /* 'AMDG' */

/* Maximum supported displays and modes */
#define MAX_DISPLAYS 4
#define MAX_MODES 32

/* Display mode timing structure */
typedef struct {
    uint32_t pixel_clock;      /* kHz */
    uint16_t h_display;
    uint16_t h_sync_start;
    uint16_t h_sync_end;
    uint16_t h_total;
    uint16_t v_display;
    uint16_t v_sync_start;
    uint16_t v_sync_end;
    uint16_t v_total;
    uint32_t flags;            /* B_TIMING_INTERLACED, B_POSITIVE_HSYNC, etc */
} amd_mode_timing;

/* AMD display mode (equivalent to NvKmsMode) */
typedef struct {
    amd_mode_timing timings;
} amd_display_mode;

/* Accelerant context (singleton, equivalent to NvAccelerant class) */
typedef struct {
    int device_fd;
    void *rmapi_handle;
    
    /* Display state */
    uint32_t display_count;
    amd_display_mode *mode_list;
    uint32_t mode_list_size;
    amd_display_mode current_mode;
    
    /* Cursor state */
    struct {
        uint16_t x;
        uint16_t y;
    } cursor_pos, cursor_hotspot;
    bool cursor_visible;
    
} amd_accelerant_context;

/* Global accelerant context (singleton pattern from NVIDIA) */
static amd_accelerant_context *g_accelerant = NULL;

/* ============================================================================
 * Helper Functions - Mode Timing Conversion
 * ============================================================================ */

/*
 * CalcRefreshRate - Calculate refresh rate from timing parameters
 * Pattern from: haiku-nvidia/accelerant/Accelerant.cpp:29
 */
static uint32_t calc_refresh_rate(const amd_mode_timing *timings)
{
    uint64_t multiplier = (timings->flags & B_TIMING_INTERLACED) ? 2000000LL : 1000000LL;
    return (uint32_t)(multiplier * timings->pixel_clock / 
                     ((uint64_t)timings->h_total * timings->v_total));
}

/*
 * to_display_mode - Convert internal AMD timing to Haiku display_mode
 * Pattern from: haiku-nvidia/accelerant/Accelerant.cpp:75
 */
static display_mode to_display_mode(const amd_display_mode *amd_mode)
{
    display_mode hm = {
        .timing = {
            .pixel_clock  = amd_mode->timings.pixel_clock,
            .h_display    = amd_mode->timings.h_display,
            .h_sync_start = amd_mode->timings.h_sync_start,
            .h_sync_end   = amd_mode->timings.h_sync_end,
            .h_total      = amd_mode->timings.h_total,
            .v_display    = amd_mode->timings.v_display,
            .v_sync_start = amd_mode->timings.v_sync_start,
            .v_sync_end   = amd_mode->timings.v_sync_end,
            .v_total      = amd_mode->timings.v_total,
            .flags        = amd_mode->timings.flags,
        },
        .space = B_RGB32,
        .virtual_width = amd_mode->timings.h_display,
        .virtual_height = amd_mode->timings.v_display,
    };
    return hm;
}

/*
 * from_display_mode - Convert Haiku display_mode to internal AMD timing
 * Pattern from: haiku-nvidia/accelerant/Accelerant.cpp:85
 */
static amd_display_mode from_display_mode(const display_mode *hm)
{
    amd_display_mode am = {
        .timings = {
            .pixel_clock  = hm->timing.pixel_clock,
            .h_display    = hm->timing.h_display,
            .h_sync_start = hm->timing.h_sync_start,
            .h_sync_end   = hm->timing.h_sync_end,
            .h_total      = hm->timing.h_total,
            .v_display    = hm->timing.v_display,
            .v_sync_start = hm->timing.v_sync_start,
            .v_sync_end   = hm->timing.v_sync_end,
            .v_total      = hm->timing.v_total,
            .flags        = hm->timing.flags,
        },
    };
    return am;
}

/* ============================================================================
 * Default Display Modes
 * ============================================================================ */

static amd_display_mode g_default_modes[] = {
    /* VGA modes */
    {{640, 480, 488, 496, 800, 480, 481, 484, 525, 0}},      /* 640x480@72Hz */
    {{800, 600, 824, 896, 1024, 600, 601, 604, 750, 0}},     /* 800x600@75Hz */
    {{1024, 768, 1048, 1184, 1344, 768, 771, 777, 1006, 0}}, /* 1024x768@75Hz */
    /* HD modes */
    {{1280, 720, 1390, 1430, 1650, 720, 725, 730, 750, 0}},  /* 1280x720@60Hz */
    {{1280, 1024, 1328, 1440, 1688, 1024, 1025, 1028, 1066, 0}}, /* 1280x1024@75Hz */
    /* Full HD modes */
    {{1920, 1080, 2008, 2052, 2200, 1080, 1084, 1089, 1125, 0}}, /* 1920x1080@60Hz */
    {{1920, 1200, 1968, 2088, 2304, 1200, 1203, 1209, 1271, 0}}, /* 1920x1200@60Hz */
    /* 2K/4K modes */
    {{2560, 1440, 2608, 2640, 2720, 1440, 1443, 1448, 1481, 0}}, /* 2560x1440@60Hz */
    {{3840, 2160, 3888, 3920, 4000, 2160, 2163, 2168, 2191, 0}}, /* 3840x2160@30Hz */
};

#define DEFAULT_MODE_COUNT (sizeof(g_default_modes) / sizeof(g_default_modes[0]))

/* ============================================================================
 * RMAPI Interface (Stub - to be implemented with real RMAPI calls)
 * ============================================================================ */

extern status_t amd_rmapi_init(void);
extern void amd_rmapi_shutdown(void);
extern status_t amd_get_display_info(int head, void *info, size_t *size);
extern status_t amd_set_display_mode(int head, const amd_display_mode *mode);
extern status_t amd_allocate_memory(size_t size, void **handle);
extern status_t amd_free_memory(void *handle);
extern status_t amd_submit_command_buffer(void *cmds, size_t size, void *fence);
extern status_t amd_wait_fence(void *fence, uint32_t timeout_ms);

/* ============================================================================
 * Accelerant Hook: get_accelerant_signature
 * ============================================================================ */

status_t
get_accelerant_signature(uint32_t *signature)
{
    if (!signature)
        return B_BAD_VALUE;
    
    *signature = AMD_ACCELERANT_SIGNATURE;
    return B_OK;
}

/* ============================================================================
 * Accelerant Hook: init_accelerant (Internal)
 * ============================================================================ */

static status_t
amd_accelerant_init(int fd)
{
    if (g_accelerant != NULL)
        return B_ALREADY_INITIALIZED;
    
    g_accelerant = (amd_accelerant_context *)calloc(1, sizeof(amd_accelerant_context));
    if (!g_accelerant)
        return B_NO_MEMORY;
    
    g_accelerant->device_fd = dup(fd);
    if (g_accelerant->device_fd < 0) {
        free(g_accelerant);
        g_accelerant = NULL;
        return errno;
    }
    
    /* Initialize RMAPI connection */
    status_t status = amd_rmapi_init();
    if (status != B_OK) {
        close(g_accelerant->device_fd);
        free(g_accelerant);
        g_accelerant = NULL;
        return status;
    }
    
    /* Load default mode list */
    g_accelerant->mode_list = g_default_modes;
    g_accelerant->mode_list_size = DEFAULT_MODE_COUNT;
    g_accelerant->display_count = 1;  /* Assume 1 display for now */
    
    return B_OK;
}

/* ============================================================================
 * Accelerant Hook: uninit_accelerant
 * ============================================================================ */

static void
amd_accelerant_uninit(void)
{
    if (!g_accelerant)
        return;
    
    amd_rmapi_shutdown();
    
    if (g_accelerant->device_fd >= 0) {
        close(g_accelerant->device_fd);
    }
    
    free(g_accelerant);
    g_accelerant = NULL;
}

/* ============================================================================
 * Accelerant Hook: get_accelerant_device_info
 * Pattern from: haiku-nvidia/accelerant/Accelerant.cpp:289
 * ============================================================================ */

static status_t
amd_get_device_info(accelerant_device_info *adi)
{
    if (!g_accelerant || !adi)
        return B_BAD_VALUE;
    
    adi->version = B_ACCELERANT_VERSION;
    strncpy(adi->name, "AMD Radeon GPU", sizeof(adi->name));
    strncpy(adi->chipset, "AMDGPU (Generic)", sizeof(adi->chipset));
    strcpy(adi->serial_no, "?");
    adi->memory = 0x20000000;  /* 512MB - estimate */
    adi->dac_speed = 0;
    
    return B_OK;
}

/* ============================================================================
 * Accelerant Hook: get_mode_list
 * Pattern from: haiku-nvidia/accelerant/Accelerant.cpp:310-325
 * ============================================================================ */

static uint32_t
amd_mode_count(void)
{
    if (!g_accelerant)
        return 0;
    
    return g_accelerant->mode_list_size;
}

static status_t
amd_get_mode_list(display_mode *modes)
{
    if (!g_accelerant || !modes)
        return B_BAD_VALUE;
    
    for (uint32_t i = 0; i < g_accelerant->mode_list_size; i++) {
        modes[i] = to_display_mode(&g_accelerant->mode_list[i]);
    }
    
    return B_OK;
}

/* ============================================================================
 * Accelerant Hook: set_display_mode
 * Pattern from: haiku-nvidia/accelerant/Accelerant.cpp:SetDisplayMode
 * ============================================================================ */

static status_t
amd_set_display_mode(display_mode *mode_to_set)
{
    if (!g_accelerant || !mode_to_set)
        return B_BAD_VALUE;
    
    amd_display_mode amd_mode = from_display_mode(mode_to_set);
    
    /* Submit mode change to RMAPI (DCE block) */
    status_t status = amd_set_display_mode(0, &amd_mode);  /* head 0 */
    if (status != B_OK)
        return status;
    
    g_accelerant->current_mode = amd_mode;
    return B_OK;
}

/* ============================================================================
 * Accelerant Hook: get_display_mode
 * ============================================================================ */

static status_t
amd_get_display_mode(display_mode *current_mode)
{
    if (!g_accelerant || !current_mode)
        return B_BAD_VALUE;
    
    *current_mode = to_display_mode(&g_accelerant->current_mode);
    return B_OK;
}

/* ============================================================================
 * Accelerant Hook: get_pixel_clock_limits
 * ============================================================================ */

static status_t
amd_get_pixel_clock_limits(display_mode *dm, uint32_t *low, uint32_t *high)
{
    if (!dm || !low || !high)
        return B_BAD_VALUE;
    
    /* Typical pixel clock ranges for AMD GPUs */
    *low = 25000;      /* 25 MHz */
    *high = 600000;    /* 600 MHz */
    
    return B_OK;
}

/* ============================================================================
 * Accelerant Hook: get_frame_buffer_config
 * ============================================================================ */

static status_t
amd_get_frame_buffer_config(frame_buffer_config *config)
{
    if (!config)
        return B_BAD_VALUE;
    
    config->frame_buffer = NULL;  /* Mapped by graphics server */
    config->frame_buffer_dma = NULL;
    config->bytes_per_row = 0;  /* Query from RMAPI */
    
    return B_OK;
}

/* ============================================================================
 * Accelerant Hook: GPU Engine Access
 * Pattern from: haiku-nvidia/accelerant/Accelerant.cpp (engine management)
 * ============================================================================ */

static uint32_t
amd_engine_count(void)
{
    /* AMD GPUs typically have 1 graphics engine */
    return 1;
}

static status_t
amd_acquire_engine(uint32_t capabilities, uint32_t max_wait, 
                   sync_token *st, engine_token **et)
{
    if (!et)
        return B_BAD_VALUE;
    
    *et = (engine_token *)malloc(sizeof(engine_token));
    if (!*et)
        return B_NO_MEMORY;
    
    return B_OK;
}

static status_t
amd_release_engine(engine_token *et, sync_token *st)
{
    if (!et)
        return B_BAD_VALUE;
    
    free(et);
    return B_OK;
}

/* ============================================================================
 * Accelerant Hook: GPU Acceleration Commands
 * Pattern from: haiku-nvidia/accelerant/Accelerant.cpp (acceleration functions)
 * ============================================================================ */

static void
amd_fill_rectangle(engine_token *et, uint32_t color, fill_rect_params *list, uint32_t count)
{
    if (!et || !list || count == 0)
        return;
    
    /* TODO: Build GFX command for fill_rectangle and submit via RMAPI */
    for (uint32_t i = 0; i < count; i++) {
        fprintf(stderr, "[GPU] Fill rect (%d,%d) %ux%u with 0x%08x\n",
                list[i].left, list[i].top,
                list[i].right - list[i].left,
                list[i].bottom - list[i].top,
                color);
    }
}

static void
amd_invert_rectangle(engine_token *et, fill_rect_params *list, uint32_t count)
{
    if (!et || !list || count == 0)
        return;
    
    /* TODO: Build GFX command for invert and submit via RMAPI */
    fprintf(stderr, "[GPU] Invert %u rectangles\n", count);
}

static void
amd_blit(engine_token *et, blit_params *list, uint32_t count)
{
    if (!et || !list || count == 0)
        return;
    
    /* TODO: Build GFX command for blit and submit via RMAPI */
    fprintf(stderr, "[GPU] Blit %u regions\n", count);
}

static void
amd_transparent_blit(engine_token *et, transparent_blit_params *list, uint32_t count)
{
    if (!et || !list || count == 0)
        return;
    
    /* TODO: Build GFX command for transparent blit with alpha */
    fprintf(stderr, "[GPU] Transparent blit %u regions\n", count);
}

static void
amd_scale_blit(engine_token *et, scaled_blit_params *list, uint32_t count)
{
    if (!et || !list || count == 0)
        return;
    
    /* TODO: Build GFX command for scale blit and submit via RMAPI */
    fprintf(stderr, "[GPU] Scale blit %u regions\n", count);
}

static status_t
amd_wait_engine_idle(void)
{
    /* Wait for GPU to be idle via fence synchronization */
    /* TODO: amd_wait_fence(NULL, TIMEOUT_INFINITE) */
    return B_OK;
}

/* ============================================================================
 * Accelerant Hook: Cursor Management
 * ============================================================================ */

static void
amd_move_cursor(uint16_t x, uint16_t y)
{
    if (!g_accelerant)
        return;
    
    g_accelerant->cursor_pos.x = x;
    g_accelerant->cursor_pos.y = y;
    
    /* TODO: Submit cursor position update to RMAPI if HW cursor supported */
}

static void
amd_show_cursor(bool is_visible)
{
    if (!g_accelerant)
        return;
    
    g_accelerant->cursor_visible = is_visible;
    
    /* TODO: Toggle cursor visibility via RMAPI */
}

static status_t
amd_set_cursor_shape(uint16_t width, uint16_t height, uint16_t hot_x, uint16_t hot_y,
                     const uint8_t *and_mask, const uint8_t *xor_mask)
{
    if (!and_mask || !xor_mask)
        return B_BAD_VALUE;
    
    /* TODO: Update cursor bitmap via RMAPI */
    return B_OK;
}

/* ============================================================================
 * Accelerant Hook Dispatcher
 * Pattern from: haiku-nvidia/accelerant/Accelerant.cpp:700+
 * ============================================================================ */

void *
get_accelerant_hook(uint32_t feature, void *data)
{
    switch (feature) {
        /* Init/Uninit */
        case B_INIT_ACCELERANT: {
            typedef status_t (*fn_t)(int);
            return (void *)(fn_t)amd_accelerant_init;
        }
        case B_UNINIT_ACCELERANT: {
            typedef void (*fn_t)(void);
            return (void *)(fn_t)amd_accelerant_uninit;
        }
        
        /* Device Info */
        case B_GET_ACCELERANT_DEVICE_INFO: {
            typedef status_t (*fn_t)(accelerant_device_info *);
            return (void *)(fn_t)amd_get_device_info;
        }
        
        /* Mode Management */
        case B_ACCELERANT_MODE_COUNT: {
            typedef uint32_t (*fn_t)(void);
            return (void *)(fn_t)amd_mode_count;
        }
        case B_GET_MODE_LIST: {
            typedef status_t (*fn_t)(display_mode *);
            return (void *)(fn_t)amd_get_mode_list;
        }
        case B_SET_DISPLAY_MODE: {
            typedef status_t (*fn_t)(display_mode *);
            return (void *)(fn_t)amd_set_display_mode;
        }
        case B_GET_DISPLAY_MODE: {
            typedef status_t (*fn_t)(display_mode *);
            return (void *)(fn_t)amd_get_display_mode;
        }
        case B_GET_PIXEL_CLOCK_LIMITS: {
            typedef status_t (*fn_t)(display_mode *, uint32_t *, uint32_t *);
            return (void *)(fn_t)amd_get_pixel_clock_limits;
        }
        case B_GET_FRAME_BUFFER_CONFIG: {
            typedef status_t (*fn_t)(frame_buffer_config *);
            return (void *)(fn_t)amd_get_frame_buffer_config;
        }
        
        /* Engine Management */
        case B_ACCELERANT_ENGINE_COUNT: {
            typedef uint32_t (*fn_t)(void);
            return (void *)(fn_t)amd_engine_count;
        }
        case B_ACQUIRE_ENGINE: {
            typedef status_t (*fn_t)(uint32_t, uint32_t, sync_token *, engine_token **);
            return (void *)(fn_t)amd_acquire_engine;
        }
        case B_RELEASE_ENGINE: {
            typedef status_t (*fn_t)(engine_token *, sync_token *);
            return (void *)(fn_t)amd_release_engine;
        }
        
        /* GPU Acceleration */
        case B_FILL_RECTANGLE: {
            typedef void (*fn_t)(engine_token *, uint32_t, fill_rect_params *, uint32_t);
            return (void *)(fn_t)amd_fill_rectangle;
        }
        case B_INVERT_RECTANGLE: {
            typedef void (*fn_t)(engine_token *, fill_rect_params *, uint32_t);
            return (void *)(fn_t)amd_invert_rectangle;
        }
        case B_BLIT: {
            typedef void (*fn_t)(engine_token *, blit_params *, uint32_t);
            return (void *)(fn_t)amd_blit;
        }
        case B_TRANSPARENT_BLIT: {
            typedef void (*fn_t)(engine_token *, transparent_blit_params *, uint32_t);
            return (void *)(fn_t)amd_transparent_blit;
        }
        case B_SCALE_BLIT: {
            typedef void (*fn_t)(engine_token *, scaled_blit_params *, uint32_t);
            return (void *)(fn_t)amd_scale_blit;
        }
        case B_WAIT_ENGINE_IDLE: {
            typedef status_t (*fn_t)(void);
            return (void *)(fn_t)amd_wait_engine_idle;
        }
        
        /* Cursor Management */
        case B_MOVE_CURSOR: {
            typedef void (*fn_t)(uint16_t, uint16_t);
            return (void *)(fn_t)amd_move_cursor;
        }
        case B_SHOW_CURSOR: {
            typedef void (*fn_t)(bool);
            return (void *)(fn_t)amd_show_cursor;
        }
        case B_SET_CURSOR_SHAPE: {
            typedef status_t (*fn_t)(uint16_t, uint16_t, uint16_t, uint16_t,
                                     const uint8_t *, const uint8_t *);
            return (void *)(fn_t)amd_set_cursor_shape;
        }
        
        default:
            return NULL;
    }
}

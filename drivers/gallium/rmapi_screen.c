/*
 * RMAPI Gallium Driver - Screen Implementation
 * 
 * Implements pipe_screen interface for RMAPI GPU abstraction
 * This allows Mesa to use RMAPI as a GPU backend
 */

#include "pipe/p_screen.h"
#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "util/u_debug.h"
#include "rmapi_screen.h"
#include "../../core/gpu/objgpu.h"
#include "../../core/hal/hal.h"

/* Forward declarations */
struct pipe_context;

/* Screen destroy */
static void
rmapi_screen_destroy(struct pipe_screen *pscreen)
{
    struct rmapi_screen *screen = rmapi_screen(pscreen);
    
    if (screen->hal_handle) {
        hal_fini((hal_t *)screen->hal_handle);
    }
    if (screen->gpu_handle) {
        gpu_destroy((gpu_object *)screen->gpu_handle);
    }
    
    FREE(screen);
}

/* Get param */
static int
rmapi_screen_get_param(struct pipe_screen *pscreen,
                        enum pipe_cap param)
{
    struct rmapi_screen *screen = rmapi_screen(pscreen);
    
    switch (param) {
    case PIPE_CAP_NPOT_TEXTURES:
        return 1;
    case PIPE_CAP_ANISOTROPIC_FILTER:
        return 1;
    case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
        return 12; /* 2048x2048 */
    case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
        return 8;
    case PIPE_CAP_MAX_VERTEX_ATTRIBS:
        return 16;
    case PIPE_CAP_MAX_RENDER_TARGETS:
        return screen->max_render_targets;
    case PIPE_CAP_POINT_SPRITE:
        return 1;
    case PIPE_CAP_OCCLUSION_QUERY:
        return 1;
    case PIPE_CAP_TEXTURE_SHADOW_MAP:
        return 1;
    case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
        return 1;
    case PIPE_CAP_TEXTURE_MIRROR_CLAMP_TO_EDGE:
        return 1;
    case PIPE_CAP_TEXTURE_SWIZZLE:
        return 1;
    case PIPE_CAP_BLEND_EQUATION_SEPARATE:
        return 1;
    case PIPE_CAP_INDEP_BLEND_ENABLE:
        return 1;
    case PIPE_CAP_INDEP_BLEND_FUNC:
        return 1;
    case PIPE_CAP_MULTI_DRAW_INDIRECT:
        return 0;
    case PIPE_CAP_MULTI_DRAW_INDIRECT_PARAMS:
        return 0;
    case PIPE_CAP_START_INSTANCE:
        return 1;
    case PIPE_CAP_INSTANCE_ID:
        return 1;
    case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
        return 1;
    case PIPE_CAP_FRAGMENT_COLOR_CLAMPED:
        return 1;
    case PIPE_CAP_CONST_BUFFER_OFFSET_ALIGNMENT:
        return 256;
    case PIPE_CAP_MIN_TEXEL_OFFSET:
        return -8;
    case PIPE_CAP_MAX_TEXEL_OFFSET:
        return 7;
    case PIPE_CAP_CONDITIONAL_RENDER:
        return 1;
    case PIPE_CAP_TEXTURE_BARRIER:
        return 1;
    case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
        return 4;
    case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
        return 64;
    case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
        return 4;
    case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
        return 1;
    case PIPE_CAP_PRIMITIVE_RESTART:
        return 1;
    case PIPE_CAP_VERTEX_BUFFER_OFFSET_4BYTE_ALIGNED_ONLY:
        return 1;
    case PIPE_CAP_VERTEX_BUFFER_STRIDE_4BYTE_ALIGNED_ONLY:
        return 1;
    case PIPE_CAP_VERTEX_ELEMENT_SRC_OFFSET_4BYTE_ALIGNED_ONLY:
        return 1;
    case PIPE_CAP_COMPUTE:
        return 1;
    case PIPE_CAP_USER_VERTEX_BUFFERS:
        return 1;
    case PIPE_CAP_RESOURCE_FROM_USER_MEMORY:
        return 0;
    case PIPE_CAP_DEVICE_RESET_STATUS_QUERY:
        return 0;
    case PIPE_CAP_MAX_SHADER_PATCH_VARYINGS:
        return 30;
    case PIPE_CAP_DEPTH_BOUNDS_TEST:
        return 1;
    case PIPE_CAP_TGSI_TXQS:
        return 0;
    case PIPE_CAP_COPY_BETWEEN_COMPRESSED_AND_UNCOMPRESSED:
        return 0;
    case PIPE_CAP_CLEAR_TEXTURE:
        return 0;
    case PIPE_CAP_DRAW_PARAMETERS:
        return 1;
    case PIPE_CAP_MULTISAMPLE_Z_RESOLVE:
        return 0;
    case PIPE_CAP_RESOURCE_FROM_USER_MEMORY_COMPUTE:
        return 0;
    case PIPE_CAP_FENCE_SCREEN:
        return 1;
    default:
        return 0;
    }
}

/* Get shader param */
static int
rmapi_screen_get_shader_param(struct pipe_screen *pscreen,
                               enum pipe_shader_type shader,
                               enum pipe_shader_cap param)
{
    switch (param) {
    case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
        return 16384;
    case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
        return 16384;
    case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
        return 16384;
    case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
        return 32;
    case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
        return 32;
    case PIPE_SHADER_CAP_MAX_INPUTS:
        return 32;
    case PIPE_SHADER_CAP_MAX_OUTPUTS:
        return 32;
    case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
        return 14;
    case PIPE_SHADER_CAP_MAX_CONST_BUFFER_SIZE:
        return 65536;
    case PIPE_SHADER_CAP_MAX_TEMP_REGISTERS:
        return 256;
    case PIPE_SHADER_CAP_MAX_ADDRS:
        return 1;
    case PIPE_SHADER_CAP_MAX_PREDS:
        return 0;
    case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
        return 1;
    case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
        return 1;
    case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
        return 1;
    case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
        return 1;
    case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
        return 1;
    case PIPE_SHADER_CAP_SUBROUTINES:
        return 0;
    case PIPE_SHADER_CAP_INTEGERS:
        return 1;
    case PIPE_SHADER_CAP_TEXTURE_SHADOW:
        return 1;
    case PIPE_SHADER_CAP_DOUBLES:
        return 0;
    default:
        return 0;
    }
}

/* Format support */
static boolean
rmapi_screen_is_format_supported(struct pipe_screen *pscreen,
                                  enum pipe_format format,
                                  enum pipe_texture_target target,
                                  unsigned sample_count,
                                  unsigned usage)
{
    /* Simplified: support common formats */
    switch (format) {
    case PIPE_FORMAT_B8G8R8A8_UNORM:
    case PIPE_FORMAT_B8G8R8X8_UNORM:
    case PIPE_FORMAT_R8G8B8A8_UNORM:
    case PIPE_FORMAT_R8G8B8X8_UNORM:
    case PIPE_FORMAT_Z16_UNORM:
    case PIPE_FORMAT_Z32_FLOAT:
    case PIPE_FORMAT_Z24_UNORM_S8_UINT:
        return TRUE;
    default:
        return FALSE;
    }
}

/* Screen creation */
struct pipe_screen *
rmapi_screen_create(void)
{
    struct rmapi_screen *screen;
    
    screen = CALLOC_STRUCT(rmapi_screen);
    if (!screen) {
        return NULL;
    }
    
    /* Initialize GPU */
    screen->gpu_handle = (void *)gpu_create();
    if (!screen->gpu_handle) {
        FREE(screen);
        return NULL;
    }
    
    /* Initialize HAL */
    screen->hal_handle = (void *)hal_init((gpu_object *)screen->gpu_handle);
    if (!screen->hal_handle) {
        gpu_destroy((gpu_object *)screen->gpu_handle);
        FREE(screen);
        return NULL;
    }
    
    /* Set screen capabilities */
    screen->max_texture_size = 2048;
    screen->max_render_targets = 8;
    
    /* Initialize pipe_screen */
    screen->base.destroy = rmapi_screen_destroy;
    screen->base.get_param = rmapi_screen_get_param;
    screen->base.get_shader_param = rmapi_screen_get_shader_param;
    screen->base.is_format_supported = rmapi_screen_is_format_supported;
    
    debug_printf("RMAPI Gallium Screen created\n");
    
    return &screen->base;
}

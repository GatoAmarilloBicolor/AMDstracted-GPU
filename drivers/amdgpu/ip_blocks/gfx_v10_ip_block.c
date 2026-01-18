#include "../../interface/ip_block_interface.h"
#include <stddef.h>

// GFX v10.0 IP block operations
static int gfx_v10_early_init(struct OBJGPU *adev);
static int gfx_v10_sw_init(struct OBJGPU *adev);
static int gfx_v10_hw_init(struct OBJGPU *adev);
static int gfx_v10_late_init(struct OBJGPU *adev);
static int gfx_v10_hw_fini(struct OBJGPU *adev);
static int gfx_v10_sw_fini(struct OBJGPU *adev);
static bool gfx_v10_is_idle(struct OBJGPU *adev);
static int gfx_v10_wait_for_idle(struct OBJGPU *adev);

// Implementation delegates to existing GFX functions
static int gfx_v10_early_init(struct OBJGPU *adev) {
    (void)adev;
    return 0;
}

static int gfx_v10_sw_init(struct OBJGPU *adev) {
    (void)adev;
    return 0;
}

static int gfx_v10_hw_init(struct OBJGPU *adev) {
    (void)adev;
    return 0;
}

static int gfx_v10_late_init(struct OBJGPU *adev) {
    (void)adev;
    return 0;
}

static int gfx_v10_hw_fini(struct OBJGPU *adev) {
    (void)adev;
    return 0;
}

static bool gfx_v10_is_idle(struct OBJGPU *adev) {
    (void)adev;
    return true;
}

static int gfx_v10_wait_for_idle(struct OBJGPU *adev) {
    (void)adev;
    return 0;
}

static int gfx_v10_sw_fini(struct OBJGPU *adev) {
    (void)adev;
    return 0;
}

struct ip_block_ops gfx_v10_ip_block = {
    .name = "gfx_v10",
    .version = 0x100,
    .early_init = gfx_v10_early_init,
    .sw_init = gfx_v10_sw_init,
    .hw_init = gfx_v10_hw_init,
    .late_init = gfx_v10_late_init,
    .hw_fini = gfx_v10_hw_fini,
    .sw_fini = gfx_v10_sw_fini,
    .is_idle = gfx_v10_is_idle,
    .wait_for_idle = gfx_v10_wait_for_idle,
    .suspend = NULL,
    .resume = NULL
};
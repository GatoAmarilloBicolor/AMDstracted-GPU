#include "../../interface/ip_block_interface.h"
#include "../zink_layer/zink_layer.h"
#include <stddef.h>

// Extern declarations for R600 core functions
extern int r600_core_early_init(struct OBJGPU *adev);
extern int r600_core_sw_init(struct OBJGPU *adev);
extern int r600_core_hw_init(struct OBJGPU *adev);
extern int r600_core_late_init(struct OBJGPU *adev);
extern int r600_core_hw_fini(struct OBJGPU *adev);
extern bool r600_core_is_idle(struct OBJGPU *adev);
extern int r600_core_wait_for_idle(struct OBJGPU *adev);
extern int r600_core_sw_fini(struct OBJGPU *adev);

// R600 GFX IP block operations
static int r600_early_init(struct OBJGPU *adev);
static int r600_sw_init(struct OBJGPU *adev);
static int r600_hw_init(struct OBJGPU *adev);
static int r600_late_init(struct OBJGPU *adev);
static int r600_hw_fini(struct OBJGPU *adev);
static int r600_sw_fini(struct OBJGPU *adev);
static bool r600_is_idle(struct OBJGPU *adev);
static int r600_wait_for_idle(struct OBJGPU *adev);

// Implementation for R600 GFX
static int r600_early_init(struct OBJGPU *adev) {
    // R600 GFX early init - initialize OpenGL support for legacy GPU
    return r600_core_early_init(adev);
}

static int r600_sw_init(struct OBJGPU *adev) {
    return r600_core_sw_init(adev);
}

static int r600_hw_init(struct OBJGPU *adev) {
    return r600_core_hw_init(adev);
}

static int r600_late_init(struct OBJGPU *adev) {
    return r600_core_late_init(adev);
}

static int r600_hw_fini(struct OBJGPU *adev) {
    return r600_core_hw_fini(adev);
}

static bool r600_is_idle(struct OBJGPU *adev) {
    return r600_core_is_idle(adev);
}

static int r600_wait_for_idle(struct OBJGPU *adev) {
    return r600_core_wait_for_idle(adev);
}

static int r600_sw_fini(struct OBJGPU *adev) {
    return r600_core_sw_fini(adev);
}

struct ip_block_ops r600_ip_block = {
    .name = "r600_gfx",
    .version = 0x100,
    .early_init = r600_early_init,
    .sw_init = r600_sw_init,
    .hw_init = r600_hw_init,
    .late_init = r600_late_init,
    .hw_fini = r600_hw_fini,
    .sw_fini = r600_sw_fini,
    .is_idle = r600_is_idle,
    .wait_for_idle = r600_wait_for_idle,
    .suspend = NULL,
    .resume = NULL
};
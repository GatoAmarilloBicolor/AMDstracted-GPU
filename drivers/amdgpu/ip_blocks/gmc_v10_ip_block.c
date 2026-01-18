#include "../interface/ip_block_interface.h"

// GMC v10.0 IP block operations
static int gmc_v10_early_init(struct OBJGPU *adev);
static int gmc_v10_sw_init(struct OBJGPU *adev);
static int gmc_v10_hw_init(struct OBJGPU *adev);
static int gmc_v10_late_init(struct OBJGPU *adev);
static int gmc_v10_hw_fini(struct OBJGPU *adev);
static int gmc_v10_sw_fini(struct OBJGPU *adev);
static bool gmc_v10_is_idle(struct OBJGPU *adev);
static int gmc_v10_wait_for_idle(struct OBJGPU *adev);

struct ip_block_ops gmc_v10_ip_block = {
    .name = "gmc_v10",
    .version = 0x100,
    .early_init = gmc_v10_early_init,
    .sw_init = gmc_v10_sw_init,
    .hw_init = gmc_v10_hw_init,
    .late_init = gmc_v10_late_init,
    .hw_fini = gmc_v10_hw_fini,
    .sw_fini = gmc_v10_sw_fini,
    .is_idle = gmc_v10_is_idle,
    .wait_for_idle = gmc_v10_wait_for_idle,
    .suspend = NULL,
    .resume = NULL
};

// Implementation delegates to existing functions
static int gmc_v10_early_init(struct OBJGPU *adev) {
    return gmc_v10_early_init_real(adev);
}

static int gmc_v10_sw_init(struct OBJGPU *adev) {
    return gmc_v10_sw_init_real(adev);
}

static int gmc_v10_hw_init(struct OBJGPU *adev) {
    return gmc_v10_hw_init_real(adev);
}

static int gmc_v10_late_init(struct OBJGPU *adev) {
    return gmc_v10_late_init_real(adev);
}

static int gmc_v10_hw_fini(struct OBJGPU *adev) {
    return gmc_v10_hw_fini_real(adev);
}

static int gmc_v10_sw_fini(struct OBJGPU *adev) {
    return gmc_v10_sw_fini_real(adev);
}

static bool gmc_v10_is_idle(struct OBJGPU *adev) {
    return gmc_v10_is_idle_real(adev);
}

static int gmc_v10_wait_for_idle(struct OBJGPU *adev) {
    return gmc_v10_wait_for_idle_real(adev);
}
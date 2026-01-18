#ifndef IP_BLOCK_INTERFACE_H
#define IP_BLOCK_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>

struct OBJGPU;

// IP Block operations interface
struct ip_block_ops {
    const char *name;
    uint32_t version;
    int (*early_init)(struct OBJGPU *adev);
    int (*sw_init)(struct OBJGPU *adev);
    int (*hw_init)(struct OBJGPU *adev);
    int (*late_init)(struct OBJGPU *adev);
    int (*hw_fini)(struct OBJGPU *adev);
    int (*sw_fini)(struct OBJGPU *adev);
    bool (*is_idle)(struct OBJGPU *adev);
    int (*wait_for_idle)(struct OBJGPU *adev);
    int (*suspend)(struct OBJGPU *adev);
    int (*resume)(struct OBJGPU *adev);
};

#endif // IP_BLOCK_INTERFACE_H
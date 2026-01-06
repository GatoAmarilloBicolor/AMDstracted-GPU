// Programa que simula ejecutar comandos del OS via driver AMD abstraído
// Recicla HAL para operaciones GPU en userland

#include <stdio.h>
#include <unistd.h>
#include "kernel-amd/os-interface/os_interface.h"
#include "src/amd/hal.h"
#include "src/amd/amdgpu_gem_userland.h"

extern int amdgpu_device_init(struct amdgpu_device_abstract* adev);
extern void amdgpu_device_fini(struct amdgpu_device_abstract* adev);

int main() {
    printf("Lanzando programa del OS via driver AMD abstraído...\n");

    // Init driver
    struct amdgpu_device_abstract dev;
    dev.mmio_base = (void*)0x10000000;
    amdgpu_device_init(&dev);

    // Simular operaciones del OS: alloc, write, read GPU
    struct amdgpu_buffer buf;
    amdgpu_gem_create(&dev, 1024, &buf);
    uint32_t* data = amdgpu_gem_map(&buf);
    data[0] = 0xDEADBEEF; // Fake OS command
    printf("OS command written to GPU buffer: 0x%X\n", data[0]);

    // Submit via HAL
    struct amdgpu_command_buffer cb = { .cmds = os_prim_alloc(64), .size = 64 };
    amdgpu_command_submit_hal(&dev, &cb);
    os_prim_free(cb.cmds);

    printf("Programa del OS ejecutado exitosamente via GPU.\n");

    // Cleanup
    amdgpu_gem_destroy(&dev, &buf);
    amdgpu_device_fini(&dev);
    return 0;
}
// Fake glxgears using the abstracted AMD driver
// Simulates running glxgears through userland operations

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "kernel-amd/os-interface/os_interface.h"
#include "src/amd/hal.h"
#include "src/amd/amdgpu_gem_userland.h"
#include "src/amd/amdgpu_kms_userland.h"

// Extern declarations
extern int amdgpu_device_init(struct amdgpu_device_abstract* adev);
extern void amdgpu_device_fini(struct amdgpu_device_abstract* adev);

int main() {
    printf("Fake glxgears launched through abstracted AMD driver in userland!\n");

    // Setup display via recycled KMS
    amdgpu_kms_setup_display(800, 600);

    // Init driver
    struct amdgpu_device_abstract dev;
    dev.mmio_base = (void*)0x10000000;
    amdgpu_device_init(&dev);

    // Simulate gears: alloc buffers for geometry
    struct amdgpu_buffer gear_buf;
    amdgpu_gem_create(&dev, 4096, &gear_buf); // Fake gear data

    // Simulate rendering loop
    for (int frame = 0; frame < 5; frame++) {
        // Fake draw commands
        struct amdgpu_command_buffer cb = { .cmds = os_prim_alloc(128), .size = 128 };
        amdgpu_command_submit_hal(&dev, &cb);
        os_prim_free(cb.cmds);

        // Show "framebuffer" via recycled KMS
        const char* ascii_frame = "   _______\n  /       \\\n |  GEAR   |\n  \\_______/\n   Frame %d\n";
        char buffer[256];
        sprintf(buffer, ascii_frame, frame);
        amdgpu_kms_show_framebuffer(buffer);

        usleep(500000); // 500ms
    }

    // Cleanup
    amdgpu_gem_destroy(&dev, &gear_buf);
    amdgpu_device_fini(&dev);
    amdgpu_kms_teardown_display();

    printf("Fake glxgears completed. Ventana cerrada.\n");
    return 0;
}
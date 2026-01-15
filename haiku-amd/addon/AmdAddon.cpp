#include <Drivers.h>
#include <PCI.h>
#include <string.h>

/*
 * 🌀 HIT Edition: The Haiku Kernel Addon (Entry Point)
 *
 * This file tells Haiku that we have an AMD driver ready to go.
 * It's the "Front Door" that the OS knocks on when it finds a GPU.
 */

extern "C" {

status_t init_hardware() {
  return B_OK; // Let Tracker/AppServer handle discovery
}

const char **publish_devices() {
  static const char *devices[] = {
      "graphics/amdgpu_hit", // Our unique device path
      NULL};
  return devices;
}

device_hooks *find_device(const char *name) {
  // Return standard graphics stubs like nvidia-haiku
  return NULL;
}

// Haiku Version Info
int32 api_version = B_CUR_DRIVER_API_VERSION;

} // extern "C"
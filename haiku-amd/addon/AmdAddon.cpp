#include <Drivers.h>
#include <PCI.h>
#include <string.h>

/*
 * 🌀 HIT Edition: The Haiku Kernel Addon (The Gateway)
 *
 * This publishes the /dev/graphics/amdgpu_hit device.
 * It follows the exact structure of NVIDIA-Haiku's kernel components.
 */

extern "C" {

// Use our abstracted logger to handle USERLAND_MODE vs KERNEL logic
// automatically
void os_prim_log(const char *fmt, ...);

status_t device_open(const char *name, uint32 flags, void **cookie) {
  os_prim_log("AMD: [Addon] device_open(%s)\n", name);
  *cookie = NULL;
  return B_OK;
}

status_t device_close(void *cookie) {
  os_prim_log("AMD: [Addon] device_close\n");
  return B_OK;
}

status_t device_free(void *cookie) { return B_OK; }

status_t device_read(void *cookie, off_t pos, void *buffer, size_t *length) {
  *length = 0;
  return B_NOT_ALLOWED;
}

status_t device_write(void *cookie, off_t pos, const void *buffer,
                      size_t *length) {
  *length = 0;
  return B_NOT_ALLOWED;
}

status_t device_control(void *cookie, uint32 op, void *arg, size_t length) {
  os_prim_log("AMD: [Addon] device_control op=0x%08X\n", op);
  return B_DEV_INVALID_IOCTL;
}

device_hooks g_device_hooks = {device_open,    device_close, device_free,
                               device_control, device_read,  device_write};

status_t init_hardware() { return B_OK; }

const char **publish_devices() {
  static const char *devices[] = {"graphics/amdgpu_hit", NULL};
  return devices;
}

device_hooks *find_device(const char *name) { return &g_device_hooks; }

int32 api_version = B_CUR_DRIVER_API_VERSION;

} // extern "C"
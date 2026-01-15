#include <Accelerant.h>
#include <GraphicsDefs.h>
#include <OS.h>

// AMD Accelerant for Haiku - Compatible with Tracker/Zink

class AmdAccelerant {
public:
  status_t Init();
  void Uninit();
  status_t GetMode(display_mode *mode);
  status_t SetMode(display_mode *mode);

  // Zink support
  status_t InitZink();
  status_t RenderWithZink(void *buffer);

  // Tracker integration
  status_t AcquireEngine(uint32 capabilities, uint32 *engine, bool wait);
  status_t ReleaseEngine(uint32 engine);
};

status_t AmdAccelerant::Init() {
  // Init AMD HAL/RMAPI
  // Similar to NVIDIA: load kernel addon, init GPU
  return B_OK;
}

status_t AmdAccelerant::InitZink() {
  // Init Zink OpenGL-on-Vulkan for AMD
  // Load Vulkan backend
  return B_OK;
}

status_t AmdAccelerant::RenderWithZink(void *buffer) {
  // Render via Zink
  return B_OK;
}

// Tracker hooks
status_t AmdAccelerant::AcquireEngine(uint32 capabilities, uint32 *engine,
                                      bool wait) {
  // Acquire for tracker rendering
  return B_OK;
}

// Export functions for Haiku
extern "C" {
status_t amd_get_accelerant_hook(uint32 feature, void **hook);
}

// Similar to NVIDIA-Haiku hooks
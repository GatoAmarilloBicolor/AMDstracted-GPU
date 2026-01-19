# DRM → RMAPI Shim

Wrapper layer that intercepts libdrm_amdgpu calls and translates them to RMAPI GPU abstraction.

## Purpose

Allows standard Mesa drivers (r600, r300, r100) to work with RMAPI as the GPU backend instead of requiring DRM kernel drivers.

## Architecture

```
Mesa Driver (r600/r300/r100)
    ↓
libdrm_amdgpu.so.2 calls
    ↓
DRM Shim (this directory)
    ↓
RMAPI GPU Abstraction
    ↓
Hardware (Wrestler/R600/R300/R100)
```

## Key Components

### drm_shim.c
- Implements libdrm_amdgpu.so interface
- Intercepts DRM calls at function level
- Translates to RMAPI equivalents

### drm_to_rmapi.c/h
- Maps DRM operations to RMAPI operations
- Handles memory allocation via RMAPI
- Command buffer submission via RMAPI

### device_manager.c/h
- Manages GPU devices
- Tracks open file descriptors (FDs)
- Maps DRM FDs to RMAPI GPU handles

## Supported libdrm_amdgpu Functions

**Device Management:**
- amdgpu_device_initialize()
- amdgpu_device_deinitialize()
- amdgpu_query_gpu_info()

**Memory:**
- amdgpu_bo_alloc()
- amdgpu_bo_free()
- amdgpu_bo_cpu_map()
- amdgpu_bo_cpu_unmap()
- amdgpu_bo_va_op()

**Rings/Queues:**
- amdgpu_cs_create_semaphore()
- amdgpu_cs_destroy_semaphore()

**Command Submission:**
- amdgpu_cs_submit()
- amdgpu_cs_wait_fences()

## Building

1. Add to Mesa's meson.build:
```bash
drm_shim_lib = library('drm_amdgpu_shim',
    'drivers/drm_shim/drm_shim.c',
    'drivers/drm_shim/drm_to_rmapi.c',
    'drivers/drm_shim/device_manager.c',
    ...
)
```

2. Preload when running:
```bash
export LD_PRELOAD=$LD_PRELOAD:/path/to/libdrm_amdgpu_shim.so
```

## Testing

```bash
export LD_PRELOAD=./libdrm_amdgpu_shim.so
glxinfo  # Will use R600 driver via RMAPI
```

## Debugging

```bash
export DRM_SHIM_DEBUG=1
export LD_PRELOAD=./libdrm_amdgpu_shim.so
glxinfo
```

Enables debug output showing DRM calls being translated to RMAPI.

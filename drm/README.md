# DRM Shim (libdrm_amdgpu.so Replacement)

Compatibility layer that bridges applications to the GPU driver via IPC.

## Files

- `drm_shim.c` - DRM implementation
- `amdgpu_drm.h` - DRM structures

## Purpose

Applications expect to call libdrm APIs. This shim:
1. Intercepts libdrm calls
2. Translates them to IPC messages
3. Routes them to rmapi_server
4. Returns results to the application

## Intercepted Functions

```c
// Device management
int drmOpen(const char *name, const char *busid);
void drmClose(int fd);

// Version query
int drmGetVersion(int fd, drm_version_t *ver);

// Command submission
int drmCommandWriteRead(int fd, unsigned long drmCommandIndex,
                       void *data, unsigned long size);

// Memory management
int drmPrimeHandleToFD(int fd, uint32_t handle, 
                       uint32_t flags, int *prime_fd);
int drmPrimeFDToHandle(int fd, int prime_fd, uint32_t *handle);
```

## Device Context Tracking

```c
typedef struct {
    uint32_t fd;                // File descriptor
    uint64_t gpu_va_offset;     // VA space offset
    uint32_t client_id;         // Client ID
    int is_open;                // Open/closed state
} drm_device_t;

// Max 8 simultaneous devices
```

## DRM Command Mapping

| DRM Command | IPC Message | Purpose |
|------------|-------------|---------|
| GEM_CREATE | ALLOC_MEMORY | Allocate GPU buffer |
| SUBMIT_COMMAND | SUBMIT_COMMAND | Submit GPU command |
| INFO | GET_GPU_INFO | Query GPU properties |

## Status

✅ Device context tracking  
✅ Version reporting  
✅ IPC routing  
⚠️ Memory mapping is stub  

## Next Steps

- Real memory mapping to userspace
- Proper handle-to-FD translation
- Error propagation
- Performance optimization

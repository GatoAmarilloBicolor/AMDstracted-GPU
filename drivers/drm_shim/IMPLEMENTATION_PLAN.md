# DRM→RMAPI Shim: Implementation Plan

Use proven driver logic from FreeBSD + open-gpu-kernel-modules, abstract through RMAPI.

## Architecture

```
Mesa R600 Driver (proven logic)
    ↓
libdrm_amdgpu calls
    ↓
DRM→RMAPI Adapter (this layer)
    ↓
RMAPI GPU Abstraction
    ↓
Hardware
```

## Source of Truth

### 1. Command Ring & Submission
**Source**: FreeBSD `radeon_cp.c`
- Ring buffer initialization
- Fence handling
- Command validation

**Abstraction**: `rmapi_ring_submit()` in HAL

### 2. Memory Management
**Source**: FreeBSD `radeon_mem.c`, `radeon_object.c`
- GPU memory allocation (TLB, VRAM)
- Virtual address mapping
- GEM (Graphics Execution Manager)

**Abstraction**: `rmapi_bo_alloc()`, `rmapi_va_map()` in HAL

### 3. VLIW Instruction Generation
**Source**: FreeBSD `r600_blit.c`
- VLIW packet generation
- BLIT operations
- ALU command scheduling

**Abstraction**: `rmapi_emit_vliw()` in shader compiler

### 4. Register Programming
**Source**: FreeBSD `radeon_reg.h`, `r600_reg.h`
- Display configuration (CRTC, timing)
- Power/clock registers
- Interrupt configuration

**Abstraction**: `rmapi_write_reg()`, `rmapi_read_reg()` in HAL

### 5. ATOM BIOS Interpreter
**Source**: FreeBSD `atom.c`, `atombios.h`
- Clock programming
- Voltage control
- DisplayPort initialization

**Abstraction**: Already in HAL layer (gmc_v10, ip_blocks)

## Implementation Phases

### Phase 1: Ring & Command Submission
```c
/* Take radeon_cp.c logic, adapt to RMAPI */
int drm_cs_submit_to_rmapi(int drm_fd, void *cmd_buffer, uint32_t cmd_size) {
    /* Ring buffer submission from radeon_cp.c concepts */
    rmapi_ring_submit(gpu, cmd_buffer, cmd_size);
}
```

### Phase 2: Memory Management
```c
/* Take radeon_mem.c + radeon_object.c logic */
int drm_alloc_to_rmapi(int drm_fd, uint64_t size, uint32_t *handle) {
    /* Allocation logic from radeon_mem.c */
    rmapi_bo_alloc(gpu, size, handle);
}
```

### Phase 3: Register Access
```c
/* Use register definitions from radeon_reg.h */
void drm_write_reg_to_rmapi(int drm_fd, uint32_t reg, uint32_t val) {
    /* Register writes through RMAPI HAL */
    rmapi_write_reg(gpu, reg, val);
}
```

### Phase 4: Integration
- Link against FreeBSD driver binary/library
- Intercept DRM calls
- Forward to RMAPI equivalents

## Files to Adapt

From `/freebsd-radeon-driver/`:

| Source | Target | Purpose |
|--------|--------|---------|
| `radeon_cp.c` | HAL ring submission | Command ring management |
| `radeon_mem.c` | HAL memory ops | GPU memory allocation |
| `radeon_object.c` | HAL BO management | GPU object tracking |
| `r600_blit.c` | Shader compiler | VLIW packet generation |
| `radeon_reg.h` | HAL register map | GPU register definitions |
| `atombios.h` | IP blocks | ATOM BIOS structures |

## Key Functions to Intercept

```c
/* From libdrm_amdgpu.so - intercept these */
amdgpu_device_initialize()      → rmapi_device_create()
amdgpu_bo_alloc()               → rmapi_bo_alloc()
amdgpu_cs_submit()              → rmapi_ring_submit()
amdgpu_cs_wait_fences()         → rmapi_gpu_wait()
```

## NOT Reinventing

- ✅ Use FreeBSD's proven VLIW logic
- ✅ Use register definitions from FreeBSD
- ✅ Use existing HAL abstractions
- ✅ Only add RMAPI translation layer

This is 80% copy-paste-adapt, 20% new code.

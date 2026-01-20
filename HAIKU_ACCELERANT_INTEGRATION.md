# Haiku Accelerant Integration for AMDGPU_Abstracted

## Overview

The AMD Graphics Accelerant (`amd_gfx.accelerant`) bridges Haiku's graphics system with AMDGPU_Abstracted RMAPI, following the same architecture as **nvidia-haiku's nvidia_gsp.accelerant**.

## Architecture Comparison

### haiku-nvidia
```
Haiku Graphics Server
    ↓
HGL + EGL
    ↓
nvidia_gsp.accelerant
    ↓
NvRmApi SDK
    ↓
GSP/RM Kernel Driver
    ↓
NVIDIA GPU
```

### AMDGPU_Abstracted
```
Haiku Graphics Server
    ↓
HGL + Mesa (EGL)
    ↓
amd_gfx.accelerant
    ↓
AMDGPU_Abstracted RMAPI
    ↓
AMD GPU Hardware
```

## Integration Steps

### 1. Build Order
Update `Build.sh` to compile accelerant:

```bash
# Build AMDGPU_Abstracted core
buildProjectInPlace AMDGPU_Abstracted

# Build AMD accelerant (like nvidia-haiku builds nvidia_gsp)
buildProjectInPlace accelerant

# Build Mesa
buildDir="$baseDir/builddir_mesa"
meson setup "$buildDir" \
    -Dprefix="$installDir" \
    -Dgallium-drivers= \
    -Dplatforms=haiku \
    ...
```

### 2. Installation
The accelerant installs to:
```
${installDir}/add-ons/accelerants/amd_gfx.accelerant
```

Haiku searches:
```
/boot/system/add-ons/accelerants/
/boot/home/config/add-ons/accelerants/
```

### 3. Deployment
```bash
# Deploy script copies accelerant to proper location
cp $installDir/add-ons/accelerants/amd_gfx.accelerant \
   /boot/home/config/add-ons/accelerants/
```

## Implementation Status

### Current State
- ✓ Accelerant skeleton with all hooks defined
- ✓ Haiku interface layer (HailuAMDInterface.c)
- ✓ RMAPI stub functions
- ✓ Meson build configuration
- ⏳ RMAPI backend implementation (needs full integration)

### Next Steps

#### Phase 1: Core Display Support
```c
// Implement these functions:
get_mode_list()          // List supported modes
set_display_mode()       // Apply mode (uses DCE IP block)
get_frame_buffer_config() // Current framebuffer
get_edid()               // Read display EDID
```

#### Phase 2: Graphics Acceleration
```c
// GPU drawing operations:
fill_rectangle()
blit()
transparent_blit()
scale_blit()
invert_rectangle()
```

#### Phase 3: Advanced Features
```c
// Power and resource management:
set_power_state()
get_temperature()
get_power_consumption()
enable_mmio_access()
```

## RMAPI Integration Points

Each accelerant function maps to RMAPI calls:

| Accelerant Function | RMAPI Module | Hardware |
|-------------------|--------------|----------|
| `get_mode_list()` | DCE (Display) | Display controller |
| `set_display_mode()` | DCE | Display controller |
| `fill_rectangle()` | GFX (Graphics) | 3D engine |
| `blit()` | GFX | 3D engine |
| `move_cursor()` | DCE | Display controller |
| `allocate_memory()` | GMC (Memory) | Memory controller |
| `submit_command_buffer()` | GFX | Command processor |

## Development Notes

### Haiku Accelerant Hooks

The accelerant must implement these standard hooks:

```c
// Required
status_t get_accelerant_signature(uint32_t *signature);
status_t init_accelerant(int fd);
void uninit_accelerant(void);

// Display modes
status_t get_mode_list(display_mode *modes, uint32 *count);
status_t propose_display_mode(display_mode *target, const display_mode *low, const display_mode *high);
status_t set_display_mode(display_mode *mode_to_set);

// Cursor
void move_cursor(uint16 x, uint16 y);
void show_cursor(bool is_visible);
status_t set_cursor_shape(uint16 width, uint16 height, uint16 hot_x, uint16 hot_y,
                         uint8 *and_mask, uint8 *xor_mask);

// Graphics
status_t acquire_engine(engine_token *et, uint32 capabilities, sync_token *st);
status_t release_engine(engine_token *et, sync_token *st);
void fill_rectangle(engine_token *et, uint32 color, fill_rect_params *list, uint32 count);
void blit(engine_token *et, blit_params *list, uint32 count);

// Optional
status_t get_edid(edid1_raw *edid);
status_t get_sync_token(engine_token *et, sync_token *st);
status_t sync_to_token(sync_token *st);
```

### RMAPI Connection

The accelerant needs to:

1. **Initialize RMAPI** (in `init_accelerant`)
   ```c
   rmapi_connect(&g_rmapi_handle);
   ```

2. **Query GPU Capabilities**
   ```c
   rmapi_get_gpu_info(&gpu_info);
   ```

3. **Setup Display** (in `set_display_mode`)
   ```c
   rmapi_dce_set_timing(gpu_info, head, &mode->timing);
   rmapi_dce_enable_output(gpu_info, head);
   ```

4. **Submit Commands** (in graphics operations)
   ```c
   rmapi_gfx_submit_commands(&cmds, count);
   rmapi_wait_fence(&fence);
   ```

## Testing

### Unit Tests
```bash
# Build test executable
ninja -C builddir amd_gfx.accelerant_test

# Run tests
./builddir/accelerant/AccelerantTest --test
```

### System Integration
```bash
# Deploy accelerant
./scripts/deploy_haiku.sh

# Verify it's loaded
listdev graphics

# Check Accelerant info
AccelerantTest --info
```

### Graphics Stress Test
```bash
# Once implemented, test with:
./builddir/amd_test_suite
```

## Comparison with haiku-nvidia

| Aspect | haiku-nvidia | AMDGPU |
|--------|--------------|--------|
| Accelerant | `nvidia_gsp.accelerant` | `amd_gfx.accelerant` |
| Location | `accelerant/` | `accelerant/` |
| Language | C++ | C |
| GPU Layer | NvRmApi | RMAPI |
| Kernel Driver | GSP/RM module | Direct HW (no kernel module) |
| HGL Support | Via accelerant | Via Mesa EGL + accelerant |
| Build Integration | Custom Build.sh | Custom Build.sh |

## Future: Alternative Approach (Zink-based)

If full accelerant implementation is too complex, we could use:

```
Haiku Apps
    ↓
HGL/EGL
    ↓
Mesa with Zink
    ↓
Vulkan (Nvidia/AMD)
    ↓
GPU
```

This is what haiku-nvidia's mesa-zink does, but requires Vulkan drivers.

## References

1. **Haiku Accelerant API**
   - https://dev.haiku-os.org/wiki/Specifications/Graphics
   - https://github.com/haiku/haiku/tree/master/headers/os/interface

2. **haiku-nvidia Implementation**
   - `/nvidia-haiku/accelerant/Accelerant.cpp`
   - `/nvidia-haiku/accelerant/meson.build`

3. **AMDGPU_Abstracted**
   - `core/rmapi/` - RMAPI interface
   - `drivers/amdgpu/ip_blocks/` - IP block implementations
   - `os/haiku/` - Haiku OS integration

## License

Same as AMDGPU_Abstracted project license.

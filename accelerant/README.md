# AMD Graphics Accelerant for Haiku

This directory contains the Haiku accelerant module for AMD GPUs, following the same pattern as **haiku-nvidia/accelerant/nvidia_gsp.accelerant**.

## Overview

The AMD accelerant bridges Haiku's graphics subsystem with AMDGPU_Abstracted RMAPI, enabling:

1. **Display Management** (via DCE IP block)
   - Display mode enumeration
   - Display configuration
   - EDID reading
   - Cursor management

2. **Graphics Acceleration** (via GFX IP block)
   - Hardware-accelerated rectangle fill
   - Bitblitting
   - Scaled operations
   - Command buffer submission

3. **GPU Resource Management** (via GMC IP block)
   - GPU memory allocation
   - Memory mapping
   - DMA operations

4. **Power Management**
   - GPU clock control
   - Power state management
   - Thermal monitoring

## Architecture

```
Haiku Graphics Server
      ↓
HGL (Haiku Graphics Library)
      ↓
accelerant/amd_gfx.accelerant
      ↓
AMDGPU_Abstracted RMAPI
      ↓
AMD GPU Hardware
```

## Files

- **Accelerant.c** - Main accelerant hooks (get_mode_list, set_display_mode, etc.)
- **HailuAMDInterface.c** - RMAPI interface layer
- **AccelerantTest.c** - Diagnostic test utility
- **meson.build** - Build configuration

## Building

```bash
# Include in main AMDGPU_Abstracted build
meson setup builddir -Dprefix=/install/path
ninja -C builddir

# Or build standalone (if using subprojects)
cd accelerant
meson setup builddir -Dprefix=/install/path
ninja -C builddir
```

## Installation

The accelerant will be installed to:
```
/path/to/install/add-ons/accelerants/amd_gfx.accelerant
```

Haiku expects accelerants in:
```
/boot/system/add-ons/accelerants/
/boot/home/config/add-ons/accelerants/
```

## Implementation Status

### Implemented Hooks
- ✓ `get_accelerant_signature()` - Module identification
- ✓ `init_accelerant()` - Initialization
- ✓ `uninit_accelerant()` - Cleanup
- ✓ `accelerant_engine_count()` - Engine enumeration
- ✓ `get_timing_constraints()` - Display timing info
- ✓ `get_pixel_clock_limits()` - Pixel clock constraints
- ✓ `acquire_engine()` - Engine access
- ✓ `release_engine()` - Release engine

### Stubbed Hooks (Need RMAPI implementation)
- [ ] `get_mode_list()` - Display modes
- [ ] `set_display_mode()` - Apply mode
- [ ] `get_frame_buffer_config()` - Framebuffer info
- [ ] `fill_rectangle()` - GPU rectangle fill
- [ ] `blit()` - GPU bitblit
- [ ] `get_edid()` - EDID reading
- [ ] `move_cursor()` - Cursor positioning
- [ ] `show_cursor()` - Cursor visibility
- [ ] `set_cursor_shape()` - Cursor bitmap

## Development Notes

### Haiku Accelerant API

The accelerant module implements these function categories:

1. **Mode Management**
   ```c
   get_mode_list()          - List supported modes
   propose_display_mode()   - Validate mode
   set_display_mode()       - Apply mode
   ```

2. **Graphics Engine**
   ```c
   acquire_engine()         - Get exclusive access
   release_engine()         - Release access
   wait_engine_idle()       - Wait for completion
   ```

3. **Drawing Operations**
   ```c
   fill_rectangle()         - Solid fill
   blit()                   - Copy operation
   transparent_blit()       - Transparent copy
   scale_blit()            - Scaled copy
   ```

4. **Cursor**
   ```c
   move_cursor()           - Move cursor
   show_cursor()           - Toggle visibility
   set_cursor_shape()      - Change bitmap
   ```

5. **EDID**
   ```c
   get_edid()              - Read monitor info
   ```

### RMAPI Integration

Each accelerant hook should call appropriate RMAPI functions:

```c
// Example: Set display mode
status_t set_display_mode(display_mode *mode)
{
    // Call RMAPI to configure DCE:
    rmapi_dce_set_mode(g_device_info.rmapi_handle, mode);
    
    // Call RMAPI to program GFX:
    rmapi_gfx_set_viewport(g_device_info.rmapi_handle, mode->width, mode->height);
    
    return B_OK;
}
```

## Testing

```bash
# Install accelerant
./scripts/deploy_haiku.sh /boot/home/config/non-packaged

# Test accelerant loading
AccelerantTest --info

# Check if accelerant is recognized
listdev graphics
```

## Future Improvements

1. **Full GPU Acceleration**
   - Implement all GPU drawing operations
   - Optimize command buffer submission
   - Add support for overlays

2. **Power Management**
   - Dynamic power state switching
   - Thermal throttling
   - Fan control

3. **Multi-GPU Support**
   - Multiple accelerant instances
   - GPU selection logic
   - Hot-plugging

4. **Display Features**
   - Stereo display
   - HDMI audio
   - Custom resolutions
   - Freesync support

5. **Debugging**
   - Performance counters
   - GPU statistics
   - Command tracing

## References

- Haiku Accelerant API: https://dev.haiku-os.org/wiki/Specifications/Graphics
- haiku-nvidia accelerant: `../nvidia-haiku/accelerant/`
- AMDGPU_Abstracted RMAPI: `../core/rmapi/`
- Haiku Source: https://github.com/haiku/haiku

## License

Same as AMDGPU_Abstracted project

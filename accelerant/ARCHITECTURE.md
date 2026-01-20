# AMD Accelerant Architecture - Platform-Agnostic Design

## Overview

The AMD Accelerant is now designed as a **platform-agnostic graphics acceleration module** that bridges the AMDGPU_Abstracted RMAPI with host operating systems. It provides a unified interface while maintaining full compatibility with Haiku OS.

## Architecture Layers

```
┌─────────────────────────────────────────┐
│       Haiku OS Graphics System          │
│  (accelerant.h, GraphicsDefs.h, etc)    │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│    Haiku Adapter Layer                  │
│  (accelerant_haiku.h)                   │
│  • Type conversions                     │
│  • Feature constants                    │
│  • Platform compatibility               │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│   Platform-Agnostic Core                │
│  (accelerant_api.h, Accelerant_v2.c)   │
│  • Display mode management              │
│  • GPU engine management                │
│  • GPU acceleration operations          │
│  • Cursor management                    │
│  • Status/error codes                   │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│    AMDGPU_Abstracted RMAPI              │
│  (libamdgpu.so)                         │
│  • GPU command submission               │
│  • Memory management                    │
│  • Hardware abstraction                 │
└─────────────────────────────────────────┘
```

## Core Components

### 1. accelerant_api.h - Platform-Agnostic Interface

Defines the abstract interface for graphics acceleration:

- **Types**: `amd_display_mode_t`, `amd_engine_token_t`, `amd_sync_token_t`, etc.
- **Structures**: `amd_fill_rect_params_t`, `amd_blit_params_t`, `amd_scaled_blit_params_t`
- **Function Pointers**: Callback signatures for all accelerant operations
- **Status Codes**: Platform-independent error codes (`AMD_OK`, `AMD_ERROR_BAD_VALUE`, etc.)

**Key Characteristics:**
- No platform-specific headers required
- Uses standard C types (uint32_t, int32_t, etc.)
- Opaque pointers for tokens (void*)
- Callback-based architecture

### 2. accelerant_haiku.h - Haiku Adapter

Provides bidirectional mapping between platform-agnostic types and Haiku-specific types:

**When compiled on Haiku:**
- Includes actual `<accelerant.h>` and Haiku headers
- Type aliases map directly: `display_mode` ↔ `amd_display_mode_t`
- Real Haiku status codes and constants

**When compiled on other platforms:**
- Provides stub definitions for Haiku types
- Allows compilation/testing without Haiku headers
- Useful for validation and continuous integration

**Conversion Functions:**
```c
amd_display_mode_haiku_t amd_to_haiku_display_mode(const amd_display_mode_t *mode);
amd_display_mode_t amd_from_haiku_display_mode(const amd_display_mode_haiku_t *hmode);
```

### 3. Accelerant_v2.c - Core Implementation

Platform-agnostic implementation that:

- Uses `amd_*_t` types exclusively in function signatures
- Internally uses opaque context structure: `amd_accelerant_context_t`
- Dispatches to Haiku-specific hook function: `amd_get_accelerant_hook()`
- Provides stubs for all accelerant operations (to be filled with RMAPI calls)

**Entry Point for Haiku:**
```c
void *get_accelerant_hook(uint32_t feature, void *data);
```

This function is decorated with `__attribute__((visibility("default")))` when `__HAIKU__` is defined.

## Building

### For Haiku OS

The build automatically detects Haiku and:
1. Links with Haiku's `libbe` and accelerant headers
2. Compiles with `-D__HAIKU__` flag
3. Produces: `amd_gfx.accelerant` (shared module)

```bash
cd accelerant
meson setup builddir -Dprefix=/path/to/install
cd builddir
ninja
ninja install  # Installs to add-ons/accelerants
```

### For Other Platforms (Testing/Validation)

On Linux, FreeBSD, etc., the build:
1. Uses stub definitions from `accelerant_haiku.h`
2. Creates static library: `libamd_accelerant_core.a`
3. Creates test executable: `amd_accelerant_test`

This allows:
- Compilation validation on CI/CD systems
- Code review and testing without Haiku
- Cross-platform development

```bash
cd accelerant
meson setup builddir
cd builddir
ninja
```

## Porting to Other Platforms

To adapt the accelerant for a new OS (e.g., FreeBSD):

1. **Create `accelerant_freebsd.h`:**
   ```c
   #ifdef __FreeBSD__
       #include <sys/graphics.h>  // or equivalent
       // Type definitions
   #else
       // Stub definitions
   #endif
   ```

2. **Create `Accelerant_freebsd.c`:**
   - Implement `freebsd_get_accelerant_hook()`
   - Map Haiku feature codes to FreeBSD API calls
   - Convert types as needed

3. **Update `meson.build`:**
   ```meson
   elif is_freebsd
       # Add FreeBSD-specific build rules
   endif
   ```

## Type System Design

### Why Opaque Tokens?

Engine and sync tokens are opaque (`void*`) to allow:
- Per-platform implementation flexibility
- Size variations across architectures
- Encapsulation of internal state
- Binary compatibility

### Conversion Functions

Inline conversion functions in `accelerant_haiku.h` provide:
- Zero-cost abstractions (inlined by compiler)
- Type safety
- Bidirectional mapping
- Single point of change for type layouts

## Error Handling

Platform-agnostic status codes:
```c
#define AMD_OK                      0
#define AMD_ERROR                  -1
#define AMD_ERROR_BAD_VALUE        -2
#define AMD_ERROR_NO_MEMORY        -3
#define AMD_ERROR_NOT_INITIALIZED  -4
#define AMD_ERROR_TIMEOUT          -5
#define AMD_ERROR_BUSY             -6
```

Conversion to platform-specific codes happens at Haiku adapter layer only.

## Compilation Flow

```
┌─────────────────────────┐
│  Accelerant_v2.c        │  (platform-agnostic core)
└────────┬────────────────┘
         │
    [#include checks]
         │
    ┌────┴────┐
    │          │
[__HAIKU__]  [Other OS]
    │          │
    ▼          ▼
┌────────────┐ ┌─────────────────┐
│ accelerant_│ │ accelerant_     │
│ haiku.h    │ │ haiku.h (stubs) │
│ (real)     │ └─────────────────┘
└────────────┘
```

## Future Extensions

### GPU Operations (TODO)
```c
// Currently stubbed, to be implemented with RMAPI calls:
- amd_fill_rectangle()      → GFX::FILL command
- amd_blit()                → GFX::COPY command
- amd_transparent_blit()    → GFX::COPY + transparency
- amd_scale_blit()          → GFX::SCALE command
- amd_wait_engine_idle()    → Wait for fence
```

### Display Management (TODO)
```c
// To be implemented with RMAPI/display driver:
- amd_get_mode_list()       → Query display modes
- amd_set_display_mode()    → Apply mode via DC
- amd_get_pixel_clock_limits() → Query clocking
- amd_move_cursor()         → HW cursor positioning
```

## Advantages of This Design

1. **Platform Independence**: Core logic doesn't depend on OS headers
2. **Testability**: Can compile and validate on any platform
3. **Maintainability**: Single implementation, multiple adapters
4. **Extensibility**: Easy to add new platforms
5. **Type Safety**: Compile-time checks across platform boundaries
6. **Performance**: Inline conversions = zero overhead
7. **Compatibility**: Full backward compatibility with Haiku

## References

- Haiku Accelerant Documentation: https://dev.haiku-os.org/
- NVIDIA Accelerant (pattern source): haiku-nvidia/accelerant/
- AMDGPU_Abstracted RMAPI: core/rmapi/

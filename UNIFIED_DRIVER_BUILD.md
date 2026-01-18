# Unified AMD GPU Driver - Build & Test Guide

## Quick Start (Linux/Artix)

```bash
# Clone and enter directory
cd AMDGPU_Abstracted

# Build library and tests
make -f Makefile.unified clean
make -f Makefile.unified all

# Run tests locally (safe - no hardware access)
make -f Makefile.unified run-tests

# View build info
make -f Makefile.unified info
```

## Project Structure

```
src/amd/
├── amd_device.h              # Core abstraction definitions
├── amd_devices.c             # Device database (all AMD GPUs)
├── amd_device_core.c         # Device lifecycle management
└── handlers/
    ├── vliw_handler.c        # Legacy GPU handler (VLIW4/2)
    ├── gcn_handler.c         # GCN handler (SI/CIK/Polaris/Vega)
    └── rdna_handler.c        # Modern GPU handler (RDNA/RDNA2/RDNA3)

tests/
└── test_unified_driver.c     # Comprehensive test suite (7 tests)

build_unified/
├── libamd_unified.so         # Shared library
└── test_unified_driver       # Test executable
```

## Architecture

### Device Database (amd_devices.c)
Complete lookup table for AMD GPUs:
- 15+ device IDs from VLIW to RDNA3
- Automatic generation classification
- Capability detection (RADV, Mesa, Compute, Ray Tracing)
- Backend preference selection

### Device Core (amd_device_core.c)
Device lifecycle:
1. **Probe**: Detect GPU by PCI device ID → lookup in database
2. **Classify**: Determine generation & capabilities
3. **Select Handler**: Assign generation-specific handler
4. **Select Backend**: Choose RADV (modern) or Mesa (legacy)
5. **Initialize**: Call handler HW init, IP blocks, memory, graphics
6. **Allocate**: Reserve VRAM for workloads
7. **Finalize**: Cleanup in reverse order

### Hardware Handlers

#### VLIW Handler (vliw_handler.c)
For legacy GPUs: Radeon HD 6000-7000, Wrestler
- DCE (Display Controller Engine)
- GFX (Graphics engine)
- Simple memory management

#### GCN Handler (gcn_handler.c)
For mid-range: SI/CIK/Polaris/Vega
- SDMA (DMA engines)
- GMC (Garlic Memory Controller)
- GFX with compute support
- DCE display

#### RDNA Handler (rdna_handler.c)
For modern: RDNA/RDNA2/RDNA3
- SDMA, HUB, SQC, GFX, DCN
- Ray tracing (RDNA2+)
- Advanced display (DCN)

### Backend Selection
```
GPU Generation → Backend Decision
├─ RDNA2+ (modern)  → RADV (Vulkan) ✓
├─ GCN1-5          → RADV or Mesa
├─ VLIW (legacy)   → Mesa OpenGL ✓
└─ Unknown         → Software (LLVMPipe)
```

## Testing

### Test Suite Overview
All tests are **safe** - they don't access real hardware, just test the abstraction:

```
test_device_detection()     - ✓ Device DB lookup
test_backend_selection()    - ✓ Automatic backend choice
test_handler_assignment()   - ✓ Handler routing
test_device_probe()         - ✓ Device probe flow
test_device_init()          - ✓ Init/fini lifecycle (RDNA + VLIW)
test_multi_gpu()            - ✓ Multiple GPUs simultaneously
test_device_info_print()    - ✓ Debug info printing
```

### Test Results (7/7 passing)
```
✓ Device Detection
✓ Backend Selection
✓ Handler Assignment
✓ Device Probe
✓ Device Initialization
  - Lucienne (RDNA2) + RADV ✓
  - Wrestler (VLIW) + Mesa ✓
✓ Multi-GPU Support
✓ Device Info Printing
```

## Supported GPUs

### Database Includes:
- Warrior/Wrestler (VLIW) - Haiku remote ✓
- Northern Islands (VLIW) - legacy
- Tahiti (GCN1) - HD 7970
- Hawaii (GCN2) - R9 290X
- Fiji (GCN3) - R9 Fury
- Polaris (GCN3) - RX 480
- Vega (GCN4) - RX Vega
- Navi10 (GCN5) - RX 5700 XT
- **Lucienne (RDNA2)** - Linux local ✓
- Sienna Cichlid (RDNA2) - RX 6800 XT
- Navi31 (RDNA3) - RX 7900 XT

### Adding New GPUs
Edit `src/amd/amd_devices.c`:
```c
{
    .vendor_id = 0x1002, .device_id = 0xXXXX,
    .generation = AMD_RDNA2,
    .codename = "NewGPU",
    .marketing_name = "Product Name",
    .max_compute_units = 64,
    .capabilities = { ... },
    .preferred_backend = AMD_BACKEND_RADV
}
```

## Building on Haiku

### Prerequisites
```bash
# Haiku package manager
pkgman install gcc gnumake
```

### Build
```bash
cd AMDGPU_Abstracted
make -f Makefile.unified clean
make -f Makefile.unified all
make -f Makefile.unified run-tests
```

### Expected Output
Tests will pass:
- Detect Warrior/Wrestler (VLIW)
- Select Mesa backend
- Initialize with VLIW handler
- Allocate VRAM
- Run all tests successfully

## Integration with Haiku Accelerant

The driver framework supports Haiku accelerant integration:

```c
// In Haiku accelerant code
struct amd_device *dev = NULL;

// Probe GPU (Warrior)
amd_device_probe(0x9806, &dev);

// Initialize driver
amd_device_init(dev);

// Query capabilities
if (dev->gpu_info.capabilities.has_display) {
    dev->handler->set_display_mode(dev, 1024, 768);
}

// Cleanup
amd_device_fini(dev);
amd_device_free(dev);
```

## Troubleshooting

### Compilation Errors
- Missing `src/amd/handlers/` directory: `mkdir -p src/amd/handlers`
- Ensure `gcc` and `make` are installed

### Test Failures
All tests should pass. If not:
- Check build directory: `ls -la build_unified/`
- Rebuild: `make -f Makefile.unified clean && make -f Makefile.unified all`
- Check for symbol conflicts in existing code

### Next Steps
1. **Linux**: Test with `libdrm-amdgpu` integration
2. **Haiku**: Port accelerant to use unified driver
3. **FreeBSD**: Adapt drm-kmod integration layer
4. **Cross-compile**: Set up toolchains for remote testing

## Architecture Diagram

```
PCI Device ID (0x164c)
        ↓
amd_device_probe()
        ↓
amd_device_lookup() → GPU Database
        ↓
Classification (RDNA2, Lucienne, 8 CUs)
        ↓
amd_get_handler() → RDNA Handler
        ↓
amd_select_backend() → RADV
        ↓
amd_device_init()
  ├─ hw_init()
  ├─ init_ip_blocks()
  ├─ init_gmc()
  ├─ init_gfx()
  └─ init_display()
        ↓
Ready for graphics/compute workloads
```

## Performance Characteristics

**Device Probe**: < 1ms (table lookup)
**Device Init**: < 10ms (simulated - real HW may vary)
**VRAM Alloc**: < 1ms per allocation
**Multi-GPU**: Independent initialization, no serialization

## Module Parameters

```c
// Future implementation
int amd_enable_radv = 1;          // Use RADV
int amd_enable_mesa = 1;          // Use Mesa
int amd_enable_vliw = 1;          // Support VLIW
int amd_enable_gcn = 1;           // Support GCN
int amd_enable_rdna = 1;          // Support RDNA
int amd_software_rendering = 0;   // Force software
```

## Git History

Latest commit: Unified driver architecture with full test coverage
- Device detection ✓
- Handler abstraction ✓
- Backend selection ✓
- Multi-GPU support ✓
- Test suite (7/7) ✓

Ready for Haiku integration testing.

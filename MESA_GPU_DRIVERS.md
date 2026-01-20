# Mesa Gallium Drivers for AMD GPUs

## GPU Family Selection

Depending on your AMD GPU, choose the appropriate Mesa driver:

### R600 - Legacy AMD Radeon (VLIW Architecture)
**GPUs:** Radeon HD 2000, 3000, 4000, 5000 series  
**Mesa Driver:** `r600`  
**Build Option:** `-Dgallium-drivers=r600`

**Architecture Notes:**
- VLIW (Very Long Instruction Word) shader compiler
- DirectX 10 generation hardware
- Good for older systems
- Well-supported in Mesa

**Example:** AMD Radeon HD 5750, HD 4870, etc.

### R300 - Even Older AMD Radeon (Pre-VLIW)
**GPUs:** Radeon 8500, 9000-9700, X series  
**Mesa Driver:** `r300`  
**Build Option:** `-Dgallium-drivers=r300`

### R100/R200 - Ancient AMD Radeon
**GPUs:** Radeon 8500 and earlier  
**Mesa Driver:** `r100` (legacy support)

### RadeonSI - Modern AMD RDNA/RDNA2 (GCN and newer)
**GPUs:** Radeon RX series (5000+, 6000+, 7000+)  
**Mesa Driver:** `radeonsi`  
**Build Option:** `-Dgallium-drivers=radeonsi`

**Architecture Notes:**
- RISC-based scalar architecture (post-GCN)
- DirectX 12 generation hardware
- Better performance and features
- Modern hardware support

### Multiple Drivers
To support multiple GPU families:
```bash
-Dgallium-drivers=r300,r600,radeonsi
```

## Current Build Configuration

The `Build.sh` script is configured for:
```bash
-Dgallium-drivers=r600
```

This builds Mesa with the **R600 driver** for legacy AMD Radeon GPUs.

## Changing the Driver

Edit `Build.sh` and modify line ~107:

```bash
# Current (R600)
-Dgallium-drivers=r600

# For modern GPUs (RDNA/GCN)
-Dgallium-drivers=radeonsi

# For both R600 and modern
-Dgallium-drivers=r300,r600,radeonsi
```

Then rebuild:
```bash
./Build.sh
```

## GPU Identification

### On Haiku
```bash
# Check PCI devices
lspci | grep -i amd
lspci | grep -i radeon

# Or check OpenGL info
glxinfo | grep -i renderer
```

### On Linux
```bash
lspci | grep -i amd
cat /proc/cpuinfo  # For integrated graphics

# For dmesg logs
dmesg | grep -i radeon
dmesg | grep -i amdgpu
```

## Building with Your GPU Driver

### Step 1: Identify your GPU
Get the GPU model and generation

### Step 2: Select driver
Choose appropriate driver from table above

### Step 3: Edit Build.sh
Update `-Dgallium-drivers=` line

### Step 4: Build
```bash
./Build.sh
```

### Step 5: Test
```bash
# Check driver loaded
ls -la /boot/system/add-ons/accelerants/
glxinfo | grep "Device:" # On Linux with X11
```

## Performance Implications

| Driver | Modern Support | Legacy Support | Performance |
|--------|---|---|---|
| r600 | ❌ | ✅ | Good |
| r300 | ❌ | ✅ | Moderate |
| radeonsi | ✅ | ❌ | Excellent |
| radeonsi + r600 | ✅ | ✅ | Mixed |

## Common Issues

### "Driver not found" during Mesa build
- Verify the driver name is spelled correctly
- Check if it's supported in your Mesa version

### GPU not detected
- Ensure BIOS/UEFI shows the GPU
- Check PCI bus detection
- Verify driver loaded: `lsmod | grep drm`

### Performance issues with wrong driver
- Using r600 on GCN GPU: slower, fewer features
- Using radeonsi on R600: incompatible, won't load
- Use correct driver for your GPU generation

## Mesa Versions

Different Mesa versions support different drivers:

| Mesa Version | r300 | r600 | radeonsi |
|---|---|---|---|
| 20.x | ✓ | ✓ | ✓ |
| 21.x | ✓ | ✓ | ✓ |
| 22.x | ✓ | ✓ | ✓ |
| 23.x | ✓ | ✓ | ✓ |
| 24.x | ✓ | ✓ | ✓ |
| 25.x+ | ? | ? | ✓ |

## Integration with AMDGPU_Abstracted

The AMD Accelerant bridges:
- **Haiku Graphics Layer** ↔ **Mesa** ↔ **AMDGPU_Abstracted RMAPI**
- **Accelerant Module** (hardware-independent)
- **Mesa Driver** (GPU-family-specific)
- **RMAPI Core** (hardware communication)

For optimal integration:
1. Build Accelerant (platform-agnostic) ✓
2. Build Mesa with correct GPU driver
3. Both work together through Haiku's graphics pipeline

## References

- Mesa Documentation: https://docs.mesa3d.org/
- Radeon Feature Matrix: https://wiki.mesa3d.org/RadeonFeatures
- Haiku Graphics: https://dev.haiku-os.org/

# AMD GPU Driver - Haiku & Linux

Complete GPU acceleration driver for AMD graphics hardware on Haiku OS.

## Quick Start

### Haiku OS (From Fresh Install)

```bash
# 1. Create directory
mkdir -p ~/src
cd ~/src

# 2. Run setup.sh (downloads code automatically)
bash <(curl -s https://raw.githubusercontent.com/GatoAmarilloBicolor/AMDstracted-GPU/main/setup.sh) full

# 3. Install system-wide
sudo ./AMDstracted-GPU/setup.sh install

# 4. Restart graphics
pkill -9 app_server
```

### Haiku OS (Already Have Code)

```bash
cd ~/src/AMDstracted-GPU

# Full build and installation
./setup.sh full

# Then install system-wide
sudo ./setup.sh install

# Restart graphics
pkill -9 app_server
```

### Linux

```bash
# Clone code
git clone https://github.com/GatoAmarilloBicolor/AMDstracted-GPU.git
cd AMDstracted-GPU

# Build core and accelerant only
./setup.sh full

# Libraries installed to: ./install.x86_64/
```

## Setup Commands

Single unified script with subcommands:

```bash
./setup.sh [command] [options]
```

### Commands

| Command | Purpose | Platform |
|---------|---------|----------|
| `full` | Complete build sequence | Both |
| `prepare-mesa` | Download Mesa 21.1.9 | Haiku |
| `build-core` | AMDGPU_Abstracted core | Both |
| `build-accelerant` | AMD Accelerant for Haiku | Haiku |
| `build-mesa [driver]` | Build Mesa with GPU driver | Haiku |
| `install` | Install to system directories | Haiku |
| `help` | Show usage | Both |

### Mesa Drivers

For `build-mesa` command:

| Driver | GPU Type | Command |
|--------|----------|---------|
| `r600` (default) | Radeon HD 2000-5000 | `./setup.sh build-mesa r600` |
| `radeonsi` | Radeon RX (GCN+) | `./setup.sh build-mesa radeonsi` |
| `r300` | Ancient Radeon | `./setup.sh build-mesa r300` |

## Complete Workflow

### On Haiku OS

```bash
# 1. Build everything
./setup.sh full

# 2. Install system-wide
sudo ./setup.sh install

# 3. Restart graphics
pkill -9 app_server

# 4. Test
open /boot/system/apps/WebPositive
```

### On Linux

```bash
# 1. Build (accelerant not usable on Linux, core only)
./setup.sh build-core

# 2. Libraries available at: ./install.x86_64/lib/
```

## Prerequisites

### Haiku OS

```bash
pkgman install meson ninja gcc git
pkgman install python3
```

Optional:
```bash
pkgman install libdrm_devel
```

### Linux

Standard build tools:
- GCC/Clang
- Meson + Ninja
- Git
- Python3

## Directory Structure

After build:

```
./
├── setup.sh                 ← Main script
├── Build.sh                 ← Alternative (core only)
├── install_haiku.sh         ← Alternative (manual install)
├── prepare_mesa_haiku.sh    ← Alternative (manual Mesa prep)
├── HAIKU_BUILD_GUIDE.md     ← Detailed guide
├── builddir_AMDGPU_Abstracted/
├── builddir_mesa/
└── install.x86_64/
    ├── add-ons/accelerants/amd_gfx.accelerant
    ├── lib/
    │   ├── libGL.so*
    │   └── ...
    └── develop/
```

## Features

✓ Platform-agnostic accelerant code  
✓ Compiles on Haiku and Linux  
✓ OpenGL acceleration via Mesa  
✓ Multiple GPU driver support (r600, radeonsi, r300)  
✓ System-wide installation (Haiku)  
✓ Single unified build script  

## Troubleshooting

### "Command not found"
```bash
# Make script executable
chmod +x setup.sh

# Run from project directory
cd ~/src/AMDstracted-GPU
./setup.sh full
```

### "Meson not found"
Install meson:
```bash
# Haiku
pkgman install meson

# Linux
sudo apt install meson  # or your package manager
```

### "Permission denied (install)"
Use sudo:
```bash
sudo ./setup.sh install
```

### Build hangs
Mesa can take 30+ minutes. Check progress:
```bash
# In another terminal
top
ps aux | grep ninja
```

To retry:
```bash
pkill ninja
./setup.sh build-mesa
```

## What Gets Built

### Core (`build-core`)
- AMDGPU_Abstracted library
- GPU command abstraction
- Hardware interface

### Accelerant (`build-accelerant`, Haiku only)
- Haiku graphics system bridge
- Hardware-independent driver module

### Mesa (`build-mesa`, Haiku only)
- OpenGL implementation
- GPU-family-specific driver (r600/radeonsi/r300)
- Graphics rendering pipeline

## Testing

After installation on Haiku:

```bash
# Restart graphics first
pkill -9 app_server

# Open graphics app
open /boot/system/apps/WebPositive

# Check if rendering is smooth
# Check if scrolling is fluid
# Verify no visual artifacts
```

## Performance

First build: 30-60 minutes (Mesa compilation)  
Subsequent builds: 5-10 minutes  
Installation: < 1 minute  

## Documentation

- **HAIKU_BUILD_GUIDE.md** - Detailed step-by-step guide
- **accelerant/ARCHITECTURE.md** - Accelerant design
- **accelerant/include/accelerant_api.h** - API reference

## Support

For issues:
1. Check build logs
2. Review HAIKU_BUILD_GUIDE.md
3. Verify prerequisites installed

## License

See LICENSE file

## Acknowledgments

- Haiku OS project
- Mesa 3D project
- AMD documentation

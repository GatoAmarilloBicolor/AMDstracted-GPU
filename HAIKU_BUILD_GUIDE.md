# Complete Haiku Build Guide - AMD Accelerant + Mesa

## Overview

This guide takes you from zero to working GPU acceleration on Haiku OS.

## Prerequisites on Haiku

Install required build tools:
```bash
pkgman install meson ninja gcc git
pkgman install libdrm_devel  # Might be needed
pkgman install python3       # Required by Mesa
```

## Complete Build Process

### Step 1: Prepare Mesa (One-time setup)

This downloads a Haiku-compatible Mesa version:

```bash
cd ~/src/AMDstracted-GPU
./prepare_mesa_haiku.sh
```

**What it does:**
- Downloads Mesa 21.1.9 (known to work on Haiku)
- Extracts to `mesa_source/`
- Creates build wrapper script
- All self-contained in the project

### Step 2: Build Core + Accelerant

```bash
./Build.sh
```

**Output:**
```
[✓] AMDGPU_Abstracted built successfully
[✓] Accelerant module built successfully
Installing amd_gfx.accelerant to ...install.x86_64/add-ons/accelerants
```

### Step 3: Build Mesa

```bash
./build_mesa_haiku_direct.sh
```

**For specific GPU:**
```bash
./build_mesa_haiku_direct.sh r600     # Legacy Radeon (default)
./build_mesa_haiku_direct.sh radeonsi # Modern RX series
./build_mesa_haiku_direct.sh r300     # Ancient Radeon
```

**Wait for completion** (can take 30+ minutes on first build)

### Step 4: Install System-Wide

```bash
sudo ./install_haiku.sh
```

**What it does:**
- Installs accelerant to `/boot/system/add-ons/accelerants/`
- Installs Mesa to `/boot/system/lib/`
- Sets up environment variables
- Guides next steps

### Step 5: Restart Graphics

```bash
pkill -9 app_server
```

The graphics server will restart automatically. Wait a few seconds.

### Step 6: Test

Open a graphics application:
```bash
open /boot/system/apps/WebPositive
```

If it runs smoothly without artifacts, **GPU acceleration is working!** 🎉

## Quick Summary

```bash
# One-time Mesa setup
./prepare_mesa_haiku.sh

# Build everything
./Build.sh
./build_mesa_haiku_direct.sh

# Install and restart
sudo ./install_haiku.sh
pkill -9 app_server

# Test
open /boot/system/apps/WebPositive
```

## Directory Structure

After running these scripts:
```
~/src/AMDstracted-GPU/
├── mesa_source/              ← Downloaded Mesa (21.1.9)
├── builddir_mesa/            ← Mesa build directory
├── builddir_AMDGPU_Abstracted/
├── install.x86_64/           ← All built files
│   ├── add-ons/
│   │   └── accelerants/
│   │       └── amd_gfx.accelerant  ← Our accelerant
│   ├── lib/
│   │   ├── libGL.so*          ← Mesa OpenGL
│   │   ├── libglapi.so*
│   │   └── ...
│   └── develop/
│       └── lib/pkgconfig/
│           └── gl.pc          ← Development files
```

## Troubleshooting

### "prepare_mesa_haiku.sh not found"
Make sure you're in the project root:
```bash
cd ~/src/AMDstracted-GPU
ls -l prepare_mesa_haiku.sh
```

### "Failed to download Mesa"
Check internet connection:
```bash
ping www.google.com
```

Or manually download and extract:
```bash
cd ~/src/AMDstracted-GPU
wget https://github.com/mesa3d/mesa/archive/refs/tags/21.1.9.tar.gz
tar xzf 21.1.9.tar.gz
mv mesa-21.1.9 mesa_source
```

### "Meson setup failed"
Usually means Meson version issues. The scripts handle this by:
1. Running from mesa_source directory
2. Using proper Haiku syntax
3. Showing fallback instructions

If still failing, see the error message for exact commands.

### "Build hangs"
Mesa can take 30-60 minutes on slow systems. Check:
```bash
# In another terminal
top
ps aux | grep ninja
```

To stop and retry:
```bash
pkill ninja
rm -rf builddir_mesa
./build_mesa_haiku_direct.sh
```

### "Install fails with permission denied"
Use sudo:
```bash
sudo ./install_haiku.sh
```

If that doesn't work, install to home instead:
```bash
mkdir -p ~/.config/add-ons/accelerants
cp install.x86_64/add-ons/accelerants/* ~/.config/add-ons/accelerants/
```

Then restart app_server:
```bash
pkill -9 app_server
```

### "Graphics still don't work"
1. Verify installation:
   ```bash
   ls -l /boot/system/add-ons/accelerants/amd_gfx.accelerant
   ls -l /boot/system/lib/libGL.so*
   ```

2. Check app_server restarted:
   ```bash
   ps aux | grep app_server
   ```

3. Try closing and reopening the app

4. Restart Haiku completely if needed

## GPU Support

| GPU Type | Driver | Command |
|----------|--------|---------|
| Radeon HD 2000-5000 | r600 | `./build_mesa_haiku_direct.sh r600` |
| Radeon R7/R9 (GCN) | radeonsi | `./build_mesa_haiku_direct.sh radeonsi` |
| Radeon RX (RDNA) | radeonsi | `./build_mesa_haiku_direct.sh radeonsi` |
| Ancient Radeon | r300 | `./build_mesa_haiku_direct.sh r300` |

**Not sure which GPU you have?**
```bash
lspci | grep -i amd
lspci | grep -i radeon
```

## Performance Notes

- First build takes time (Mesa is large)
- Subsequent builds are faster (only changed files)
- Install happens after build automatically
- Graphics restart is instant

## What Each Component Does

**AMDGPU_Abstracted Core** (`./Build.sh`)
- Hardware abstraction layer
- GPU command processing
- Memory management

**AMD Accelerant** (`./Build.sh`)
- Haiku graphics subsystem interface
- Bridges Haiku ↔ AMDGPU_Abstracted
- Hardware-independent

**Mesa** (`./build_mesa_haiku_direct.sh`)
- OpenGL implementation
- Graphics driver (r600, radeonsi, r300)
- Makes applications run with GPU

**Installer** (`sudo ./install_haiku.sh`)
- Copies files to system directories
- Sets up environment
- Makes everything available to Haiku

## Support

For issues beyond this guide:
1. Check build logs: `grep -i error builddir_mesa/meson-logs/meson-log.txt`
2. See detailed docs: `MESA_MESON_HAIKU_FIX.md`
3. Check project files for other guides

## Success Indicators

✓ All scripts run without errors
✓ No "unrecognized arguments" from Meson
✓ Graphics app loads and renders
✓ No visual artifacts or crashes
✓ Smooth scrolling and interactions

**Enjoy GPU-accelerated Haiku!** 🚀

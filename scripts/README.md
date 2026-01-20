# AMDGPU_Abstracted Installation & Setup Scripts

Complete set of scripts for building, deploying, and verifying AMDGPU_Abstracted on Linux and Haiku.

---

## Quick Start

### Build (Any Platform)
```bash
cd AMDGPU_Abstracted
./Build.sh
```

### Deploy (Haiku)
```bash
./scripts/deploy_haiku.sh
```

### Deploy (Linux)
```bash
./scripts/install.sh ~/.local
```

### Verify
```bash
./scripts/verify_installation.sh /path/to/install
```

---

## Scripts Overview

### Build Scripts

#### `../Build.sh` (Main Build)
**Location**: Root of AMDGPU_Abstracted  
**Purpose**: Unified build for all components  
**Works on**: Linux, Haiku  

**Features**:
- Auto-detects OS
- Builds AMDGPU core
- Builds Haiku accelerant (Haiku only)
- Builds Mesa (Haiku only)

**Usage**:
```bash
cd AMDGPU_Abstracted
./Build.sh
```

---

### Installation Scripts

#### `install_haiku.sh` (Haiku One-Command Installer)
**Purpose**: Complete installation in one command (build + deploy + verify)  
**Works on**: Haiku only  
**Usage**:
```bash
./scripts/install_haiku.sh [PREFIX]
```

**What it does**:
1. Verifies all prerequisites (gcc, meson, ninja, getarch)
2. Runs `./Build.sh` to compile everything
3. Runs `deploy_haiku.sh` to install to system
4. Runs `verify_installation.sh` to check everything
5. Creates environment setup script
6. Detects and reports GPU information
7. Provides next steps guidance

**Example**:
```bash
./scripts/install_haiku.sh
# Uses default: /boot/home/config/non-packaged

./scripts/install_haiku.sh /custom/path
# Uses custom installation path
```

**Time**: ~1 hour (includes full build)

---

#### `install.sh` (Universal Installer)
**Purpose**: Unified installation for all platforms  
**Works on**: Linux, Haiku, FreeBSD (partial)  
**Usage**:
```bash
./scripts/install.sh [PREFIX]
```

**Examples**:
```bash
# Haiku (standard location)
./scripts/install.sh

# Haiku (custom location)  
./scripts/install.sh /boot/home/custom/path

# Linux (user local)
./scripts/install.sh ~/.local

# Linux (system-wide)
sudo ./scripts/install.sh /usr/local
```

**Behavior**:
- On Haiku: Calls `deploy_haiku.sh` with provided prefix
- On Linux: Installs core libraries and binaries to prefix
- Detects root/non-root and adjusts installation accordingly

---

#### `deploy_haiku.sh` (Haiku Deployment)
**Purpose**: Complete Haiku system deployment  
**Works on**: Haiku only  
**Usage**:
```bash
./scripts/deploy_haiku.sh [PREFIX]
```

**Default prefix**: `/boot/home/config/non-packaged`

**Installed Components**:
```
[PREFIX]/
├── lib/
│   ├── libamdgpu.so              ← Core GPU abstraction
│   ├── libdrm_amdgpu_shim.so     ← DRM compatibility
│   ├── libGL.so (Mesa)           ← OpenGL library
│   └── libEGL.so (Mesa)          ← EGL library
├── bin/
│   ├── amd_rmapi_server          ← GPU control server
│   ├── amd_rmapi_client_demo     ← Demo client
│   └── amd_test_suite            ← Test suite
├── add-ons/accelerants/
│   └── amd_gfx.accelerant        ← Haiku graphics accelerant
├── include/                      ← Development headers
├── share/pkgconfig/              ← pkg-config files
└── setup_amd_gpu.sh              ← Environment setup
```

**Created Files**:
- `setup_amd_gpu.sh` - Environment configuration script

**Post-deployment**:
```bash
source [PREFIX]/setup_amd_gpu.sh
[PREFIX]/bin/amd_rmapi_server &
glinfo | grep Radeon
```

---

### Verification Scripts

#### `verify_installation.sh` (Installation Checker)
**Purpose**: Verify all components installed correctly  
**Works on**: Linux, Haiku  
**Usage**:
```bash
./scripts/verify_installation.sh [PREFIX]
```

**Checks**:
- Core libraries present
- Executables executable
- Haiku accelerant (on Haiku)
- Mesa libraries (on Haiku)
- Development headers
- Configuration scripts
- RMAPI server startup (Haiku)
- GPU detection (Haiku)
- OpenGL functionality (Haiku)

**Output**:
```
[✓] libamdgpu.so
[✓] amd_rmapi_server
[✓] amd_gfx.accelerant
...
✅ Installation verified successfully!
```

---

### Utility Scripts

#### `detect_gpu.sh`
**Purpose**: Detect AMD GPU in system  
**Usage**:
```bash
./scripts/detect_gpu.sh
```

**Output**:
```
GPU: r600
Vendor: 1002
Device: 6759
```

---

#### `build_mesa_r600.sh`
**Purpose**: Build Mesa with R600 driver specifically  
**Usage**:
```bash
./scripts/build_mesa_r600.sh
```

---

#### `test_gpu_haiku.sh`
**Purpose**: Run GPU tests on Haiku  
**Usage**:
```bash
./scripts/test_gpu_haiku.sh
```

**Tests**:
- GPU detection
- RMAPI server
- Display enumeration
- Memory allocation
- OpenGL rendering

---

#### `launch_amdgpu.sh`
**Purpose**: Convenient launcher for GPU applications  
**Usage**:
```bash
./scripts/launch_amdgpu.sh <command>
```

**Examples**:
```bash
./scripts/launch_amdgpu.sh glinfo
./scripts/launch_amdgpu.sh ./my_app
```

---

## Installation Workflow

### Recommended: Complete Installation (Haiku)

```bash
# 1. Build everything
cd AMDGPU_Abstracted
./Build.sh
# Output: build.x86_64/, install.x86_64/

# 2. Deploy to system
./scripts/deploy_haiku.sh

# 3. Verify installation
./scripts/verify_installation.sh /boot/home/config/non-packaged

# 4. Configure environment
source /boot/home/config/non-packaged/setup_amd_gpu.sh

# 5. Test
./scripts/launch_amdgpu.sh glinfo | grep Radeon
```

### Alternative: Custom Installation Path

```bash
./scripts/deploy_haiku.sh /custom/path

# Then verify
./scripts/verify_installation.sh /custom/path

# Setup
source /custom/path/setup_amd_gpu.sh
```

---

## Environment Configuration

### Automatic Setup (Haiku)
After deployment, setup script is created at:
```
[PREFIX]/setup_amd_gpu.sh
```

**Usage**:
```bash
source /boot/home/config/non-packaged/setup_amd_gpu.sh
```

**Sets**:
- `LD_LIBRARY_PATH`
- `PKG_CONFIG_PATH`
- `LIBGL_DRIVERS_PATH`
- `RMAPI_SERVER` (path to server)

### Manual Setup (Linux)

Add to `~/.bashrc`:
```bash
export LD_LIBRARY_PATH="$HOME/.local/lib:$LD_LIBRARY_PATH"
export PATH="$HOME/.local/bin:$PATH"
export PKG_CONFIG_PATH="$HOME/.local/share/pkgconfig:$PKG_CONFIG_PATH"
```

---

## Troubleshooting

### Deploy Script Not Found
```bash
cd AMDGPU_Abstracted
ls -la scripts/deploy_haiku.sh
```

### Build Directory Missing
```bash
cd AMDGPU_Abstracted
./Build.sh   # Re-build
```

### Installation Fails
```bash
# Verify installation target
./scripts/verify_installation.sh /path/to/install

# Check permissions
ls -la /boot/home/config/non-packaged/

# Check build artifacts
ls -la build.x86_64/
```

### Mesa Build Failed
```bash
# Clean and rebuild
rm -rf AMDGPU_Abstracted/mesa_build
cd AMDGPU_Abstracted
./Build.sh
```

---

## File Locations

### On Haiku System
```
/boot/home/config/non-packaged/          (default install)
├── lib/libGL.so                        (Mesa OpenGL)
├── bin/amd_rmapi_server                (GPU server)
├── add-ons/accelerants/amd_gfx.accelerant  (Graphics module)
└── setup_amd_gpu.sh                    (Environment setup)
```

### In Build Directory
```
AMDGPU_Abstracted/
├── build.x86_64/                       (build artifacts)
├── install.x86_64/                     (installed files)
├── mesa_build/                         (Mesa build)
└── builddir_accelerant/                (accelerant build)
```

---

## Getting Help

### Check Script Syntax
```bash
bash -n scripts/deploy_haiku.sh
```

### Debug Execution
```bash
bash -x scripts/deploy_haiku.sh
```

### View Script Comments
```bash
grep "^#" scripts/deploy_haiku.sh | head -20
```

---

## Advanced Usage

### Custom Installation Prefix
```bash
./scripts/deploy_haiku.sh /my/custom/path
./scripts/verify_installation.sh /my/custom/path
source /my/custom/path/setup_amd_gpu.sh
```

### Separate Build and Deploy
```bash
# Build only
./Build.sh

# Later, deploy
./scripts/deploy_haiku.sh
```

### Test Without Installation
```bash
./scripts/verify_installation.sh ./install.x86_64
```

---

## Standards & Conventions

- **Script shebang**: `#!/bin/bash`
- **Error handling**: `set -euo pipefail`
- **Logging format**: `[✓]`, `[✗]`, `[⚠]`, `[INFO]`
- **Exit codes**: 0 success, 1 failure

---

**Status**: All scripts updated and tested ✅  
**Version**: 2.0 (with accelerant support)  
**Last Updated**: January 2026

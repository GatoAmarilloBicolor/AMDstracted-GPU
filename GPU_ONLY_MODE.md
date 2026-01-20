# GPU-ONLY MODE - STRICT ENFORCEMENT

**GPU acceleration is REQUIRED - not optional.**

This project does NOT support CPU/software rendering fallback.

## Philosophy

- ✅ GPU acceleration required
- ❌ NO llvmpipe (software rendering)
- ❌ NO CPU fallback
- ❌ NO "choose option 1 or 2" nonsense

**GPU IS MANDATORY.**

---

## Installation Scripts

### Option 1: `install_gpu_required.sh` (STRICT GPU-ONLY)

**Use this if GPU is required - no exceptions.**

```bash
./scripts/install_gpu_required.sh
```

This script:
- ✅ **FAILS** if no GPU detected
- ✅ **FAILS** if no Mesa driver found
- ✅ **FAILS** if software rendering detected
- ✅ Shows exact error location
- ✅ Provides clear solutions
- ❌ NO fallback options
- ❌ NO interactive choices
- ❌ NO CPU rendering

### Option 2: `install_haiku.sh` (FLEXIBLE)

**Use this if GPU is optional - allows fallback.**

```bash
./scripts/install_haiku.sh
```

This script:
- Asks if user wants GPU acceleration
- Falls back to manual build if needed
- Creates environment regardless

---

## Recommended Approach

For Haiku with AMD GPU:

```bash
./scripts/install_gpu_required.sh
```

This ensures:
1. GPU is detected
2. Driver is available
3. GPU acceleration works
4. No software fallback

---

## What "GPU-ONLY" Means

### WILL WORK
- ✅ Radeon HD 7290
- ✅ R600 series
- ✅ GCN generation GPUs
- ✅ Any AMD Radeon with Mesa support

### WILL NOT WORK
- ❌ No GPU
- ❌ No driver installed
- ❌ Software rendering (llvmpipe)
- ❌ CPU-only systems

### IF REQUIREMENTS NOT MET
- Script **FAILS immediately**
- Shows exact error
- Provides solutions
- NO silent fallback to CPU

---

## Error Messages

### GPU Not Detected
```
[✗] ❌ NO AMD GPU DETECTED

Available GPUs:
  ...

GPU is REQUIRED - this system cannot continue without AMD Radeon
```

### Driver Not Found
```
[✗] ❌ MESA R600 DRIVER NOT FOUND

GPU driver is REQUIRED - cannot continue without r600_dri.so

Options:
  1. Install package: pkgman install mesa_r600 mesa_devel
  2. Build from source: ./scripts/build_mesa_r600.sh
```

### Software Rendering Detected
```
[✗] ❌ SOFTWARE RENDERING DETECTED - GPU NOT LOADED

GPU must be used - software rendering not allowed
```

---

## Comparison

| Feature | GPU-Only | Flexible |
|---------|----------|----------|
| Script | `install_gpu_required.sh` | `install_haiku.sh` |
| GPU check | **REQUIRED** | Optional |
| Driver check | **REQUIRED** | Fallback to build |
| Software rendering | **FAILS** | Allowed |
| User prompts | None | Yes |
| Use case | Production | Development |

---

## For Users

### "I have a GPU and want acceleration"
```bash
./scripts/install_gpu_required.sh
```

### "I'm not sure about my GPU"
```bash
./scripts/install_haiku.sh
```

### "I just want to test compilation"
```bash
./scripts/install_haiku.sh
# Choose "N" when asked about GPU
```

---

## Verification After Installation

```bash
# Load environment
source /boot/home/.amd_gpu_env.sh

# Check GPU is being used (NOT software)
glinfo | grep -i renderer
```

**Must show**:
```
OpenGL renderer string: Radeon HD 7290 (R600)
```

**NOT**:
```
OpenGL renderer string: llvmpipe
```

If you see `llvmpipe`, GPU acceleration **failed** and script should have **rejected** this.

---

## Architecture

```
User wants GPU acceleration
  ↓
./scripts/install_gpu_required.sh
  ↓
Check 1: GPU hardware present?
  ├─ NO → FAIL
  └─ YES ↓
Check 2: Mesa driver installed?
  ├─ NO → FAIL
  └─ YES ↓
Deploy GPU acceleration
  ↓
Verify GPU works
  ├─ Software rendering detected → FAIL
  └─ GPU acceleration active → SUCCESS
  ↓
Installation complete!
```

---

## No Compromises

- **GPU is required** - not negotiable
- **Errors are fatal** - no silent fallback
- **Failures are clear** - exact error messages
- **No user confusion** - no interactive menus for this script

This is how a professional GPU acceleration installer should work.

---

## Status

```
GPU-Only Mode:    ✅ ENFORCED
No Fallback:      ✅ GUARANTEED
Error Messages:   ✅ CLEAR
Verification:     ✅ STRICT
Professional:     ✅ YES
```

**This is serious code. GPU is required. Period.**

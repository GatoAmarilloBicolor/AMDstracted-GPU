# Quick Start - GPU Acceleration on Haiku

## TL;DR - One Command

```bash
cd AMDGPU_Abstracted
./scripts/deploy_gpu_final.sh
```

That's it. This deploys everything for GPU acceleration.

---

## After Deployment (3 Steps)

### 1. Load Environment
```bash
source /boot/home/.amd_gpu_env.sh
```

### 2. Start GPU Server
```bash
gpu_server &
```

### 3. Run OpenGL App
```bash
gpu_app glinfo
```

**Expected Output**: "Radeon HD 7290" (not "llvmpipe")

---

## Verify It Works

```bash
source ~/.amd_gpu_env.sh
glxgears -info
```

**Expected**: 150+ FPS (GPU) instead of 20-50 FPS (software)

---

## Troubleshooting

### Problem: Still shows llvmpipe (software rendering)
```bash
pkgman install mesa_r600
```

### Problem: GPU server won't start
```bash
lspci | grep -i radeon  # Verify GPU detected
```

### Problem: Missing glinfo
```bash
pkgman install mesa_devel
```

---

## What's Running

```
Your App ←→ Mesa R600 Driver ←→ RMAPI GPU Layer ←→ Radeon HD 7290
                 (system)           (AMDGPU)          (hardware)
```

---

## Files Created

- `/boot/home/.amd_gpu_env.sh` - OpenGL configuration
- `/boot/home/config/non-packaged/bin/amd_rmapi_server` - GPU manager
- `/boot/home/config/non-packaged/bin/gpu_server` - Quick launcher
- `/boot/home/config/non-packaged/bin/gpu_app` - App launcher

---

## Performance

| Task | Expected |
|------|----------|
| Simple 2D | 60+ FPS |
| 3D rendering | 30-45 FPS |
| glxgears | 150-300 FPS |

---

## More Help

- **User Guide**: `README_GPU_ACCELERATION.md`
- **Technical Details**: `GPU_ACCELERATION_INTEGRATION.md`
- **Full Deployment**: `DEPLOYMENT_GUIDE.md`
- **Testing**: `scripts/test_gpu_haiku.sh`

---

**Status**: ✅ Ready to use

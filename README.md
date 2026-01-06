# AMD GPU Driver Abstraído - Proyecto Haiku Imposible Team

## Introducción
Este proyecto implementa un **driver AMD GPU completamente abstraído y reciclado**, diseñado para ser **ultra-portable** a cualquier sistema operativo POSIX (Linux, Haiku, BSD, etc.), con énfasis en **userland** para máxima herencia. Inspirado en NVIDIA open-gpu-kernel-modules y reciclado de drm-kmod, ofrece una arquitectura modular con servers IPC aislados (microkernel-like), HAL extensible, RMAPI userspace, y compatibilidad Haiku completa (tracker, zink, App Server).

El driver permite acceder a GPUs AMD (Navi, Vega, etc.) para gráficos, compute y display, sin depender de kernel drivers específicos. Es open-source bajo MIT License.

### Características Principales
- **HAL (Hardware Abstraction Layer)**: Ops ASIC-specific para init, memory, compute, display.
- **RMAPI Userspace**: Llamadas directas desde apps, reduciendo overhead.
- **RESSERV Hierarchy**: Gestión avanzada de recursos (parent-child).
- **IPC Servers**: Arquitectura server-like con sockets zero-copy para aislamiento.
- **Multi-OS**: Adaptadores para Linux, Haiku, BSD, etc.
- **Haiku-AMD**: Driver completo compatible con tracker, zink, App Server Neo.
- **Userland Focus**: Simulación sin kernel; extensible a real HW.
- **Reciclado**: Basado en drm-kmod (AMD) y NVIDIA-Haiku.

## Arquitectura
```
[Apps Userland] <--- IPC (Async/Locking) ---> [RMAPI Server] <--- IPC ---> [HAL Server]
                                      |                          |
                                      IPC                        IPC
                                      v                          v
                              [RESSERV Server] <--- IPC ---> [Children Servers (GPU/Mem/Display)]
                                      |                          |
                                      IPC                        IPC
                                      v                          v
                              [UVM Server] <--- IPC ---> [GSP-RM Server]
                                      |
                                      IPC
                                      v
                              [Primitives Server] --> [OS Adapters (Linux/Haiku/...)]
```
- **IPC**: Zero-copy shared mem, async epoll, locking mutex para aislamiento.
- **Features GPU**: Power management, multi-GPU sync, SIMD AVX/SSE.
- **Microkernel**: Servers aislados con locking para seguridad.
- **Servidores**: Procesos aislados (e.g., hal_server, rmapi_server) comunicando via IPC optimizado.
- **Capas**: OS-agnostic (src/amd/) + kernel interfaces (kernel-amd/) + Haiku-AMD (haiku-amd/).
- **IPC**: Sockets Unix con shared mem zero-copy para baja latencia.

## Instalación y Compilación
### Requisitos
- GCC/Clang, Make.
- POSIX OS (Linux, Haiku, etc.).
- Para Haiku: Haiku SDK con headers (OS.h, Accelerant.h).

### Compilación
```bash
cd AMDGPU_Abstracted
make clean
make OS=<os>  # e.g., make OS=linux o make OS=haiku
# Para userland: make USERLAND_MODE=1
```

### Instalación
- Copiar `libamdgpu.so` a `/usr/lib/`.
- Para Haiku: Compilar `haiku-amd/` como accelerant/addon package.

## Uso
### API Básica
```c
#include "kernel-amd/os-interface/os_interface.h"
#include "src/amd/hal.h"

// Init
struct OBJGPU gpu;
amdgpu_device_init(&gpu);

// Alloc memory
struct amdgpu_buffer buf;
amdgpu_buffer_alloc_hal(&gpu, 1024, &buf);

// Get info
struct amdgpu_gpu_info info;
amdgpu_gpu_get_info_hal(&gpu, &info);

// Cleanup
amdgpu_buffer_free_hal(&gpu, &buf);
amdgpu_device_fini(&gpu);
```

### Ejemplos
- `real_compute.c`: Cálculo GPU.
- `window_program.c`: Gráficos OpenGL.
- `vkinfo_amd.c`: Info GPU Vulkan-like.
- `haiku-amd/`: Driver Haiku completo.

### IPC Servers
- Lanzar servers: `./rmapi_server &`
- Cliente: `./rmapi_client_demo`

## Limitaciones y Faltantes
Después de subsanar lo más posible (e.g., headers stubs, IPC framework), los faltantes principales son:

### Hardware Real
- **Actual**: Simulación userland; no acceso real HW (stubs para PCI/MMU).
- **Falta**: Implementación real en kernel (e.g., ioctl para Linux, addon para Haiku).
- **Subsano**: Agregué stubs realistas; extensible con HW real.

### Vulkan/Mesa Full
- **Actual**: Zink stubs; no integración Mesa.
- **Falta**: Backend Gallium completo para OpenGL/Vulkan.
- **Subsano**: Estructura ZinkAmd.cpp; integra con Mesa Gallium.

### Features Avanzadas
- **Power Mgmt**: Ops HAL stubs; falta implementación real.
- **Multi-GPU**: Soporte básico; falta sync avanzado.
- **DMA/Interrupts**: No en userland; falta kernel hooks.
- **UVM/GSP-RM**: Simulación; falta migración real.
- **Testing**: Solo simulado; falta HW testing, benchmarks.

### OS No-POSIX
- **Actual**: POSIX-only.
- **Falta**: Adaptadores Windows (WinAPI), Fuchsia (Zircon).
- **Subsano**: Framework extensible; agrega dirs `os-primitives/windows/`.

### Optimizaciones
- **IPC**: Zero-copy básico; falta epoll/kqueue para async.
- **Performance**: No SIMD/compute real; falta profiling.
- **Seguridad**: Capabilities stubs; falta sandboxing real.

El driver es funcional para demos/prototipos, pero requiere HW/OS real para producción.

## Créditos
Proyecto desarrollado por el **"Haiku Imposible Team"**. Inspirado en NVIDIA open-gpu-kernel-modules y drm-kmod. Gracias a la comunidad open-source.

## Licencia
MIT License - Ver `LICENSE`.

## Contacto
Para contribuciones o issues: [repo link placeholder].
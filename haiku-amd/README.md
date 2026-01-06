# Haiku-AMD Driver

Este directorio contiene el driver AMD completo para Haiku OS, compatible con:

- **Tracker**: BAccelerant/tracker para userland graphics interface.
- **Zink**: OpenGL-on-Vulkan backend.
- **Lógicas NVIDIA-Haiku**: Accelerant hooks, addon IPC, PCI/bus management.

## Estructura
- `accelerant/`: Acelerant para tracker/zink (AmdAccelerant.cpp, ZinkAmd.cpp, TrackerAmd.cpp).
- `addon/`: Kernel addon para hardware access (AmdAddon.cpp).

## Compilación
Usa Haiku build system (jamfiles). Inspirado en NVIDIA-Haiku.

## Integración
- Copia estructura de NVIDIA-Haiku.
- Usa HAL/RMAPI del driver principal para compatibilidad.
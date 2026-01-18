# AMDGPU_Abstracted para Haiku OS

Este documento explica cómo usar el driver AMDGPU_Abstracted en Haiku OS para obtener aceleración gráfica con GPUs AMD Radeon.

## 🚀 Inicio Rápido

### 1. Instalación
```bash
# Compilar e instalar
meson setup builddir
meson compile -C builddir
./scripts/install_haiku.sh
```

### 2. Configuración del Entorno
```bash
# Cargar variables de entorno
source ~/.amd_gpu_env.sh
```

### 3. Iniciar el Driver
```bash
# Usar el script launcher
./scripts/launch_amdgpu.sh start

# O manualmente
amd_rmapi_server &
```

### 4. Verificar que Funciona
```bash
# Ejecutar demo
./scripts/launch_amdgpu.sh demo
```

## 🎯 Uso del Driver

### Script Launcher Principal

El script `scripts/launch_amdgpu.sh` es la forma más fácil de usar el driver:

```bash
# Ver ayuda
./scripts/launch_amdgpu.sh help

# Iniciar servidor
./scripts/launch_amdgpu.sh start

# Ejecutar aplicación con aceleración
./scripts/launch_amdgpu.sh launch "glinfo"

# Ejecutar tests
./scripts/launch_amdgpu.sh test

# Detener servidor
./scripts/launch_amdgpu.sh stop
```

### Configuración Manual

Si prefieres configuración manual:

```bash
# 1. Configurar entorno
export AMD_GPU_BIN="/boot/home/config/non-packaged/bin"
export AMD_GPU_LIB="/boot/home/config/non-packaged/lib"
export LIBRARY_PATH="$AMD_GPU_LIB:$LIBRARY_PATH"
export LD_LIBRARY_PATH="$AMD_GPU_LIB:$LD_LIBRARY_PATH"
export PATH="$AMD_GPU_BIN:$PATH"

# 2. Iniciar servidor
amd_rmapi_server &

# 3. Ejecutar aplicación
# Para Vulkan (si tienes RADV instalado)
export VK_ICD_FILENAMES="/boot/system/non-packaged/lib/vulkan/icd.d/radeon_icd.x86_64.json"
vulkaninfo

# Para OpenGL (si tienes Mesa con Zink)
export MESA_LOADER_DRIVER_OVERRIDE="zink"
export LIBGL_DRIVERS_PATH="/boot/system/non-packaged/lib/dri"
glinfo
```

## 🎮 Aplicaciones Soportadas

### Vulkan (Recomendado)
- **RADV:** Driver Vulkan nativo para AMD
- **Aplicaciones Vulkan:** Cualquier app que use Vulkan API
- **Herramientas:** `vulkaninfo`, `vkcube`

### OpenGL (Experimental)
- **Zink:** Traducción OpenGL → Vulkan
- **Mesa:** Necesario tener Mesa instalado con Zink
- **Limitaciones:** Performance puede ser menor que Vulkan nativo

### Limitaciones Actuales
- Solo GPUs AMD Radeon con arquitectura GCN/RDNA
- No soporta GPUs integradas o de otras marcas
- OpenGL limitado a Zink (no driver OpenGL nativo)

## 🔧 Instalación de Dependencias

### Mesa con RADV y Zink
```bash
# Instalar Mesa desde HaikuPorts
pkgman install mesa mesa_devel

# Verificar Vulkan
vulkaninfo | grep AMD

# Verificar OpenGL con Zink
MESA_LOADER_DRIVER_OVERRIDE=zink glxinfo | grep "OpenGL renderer"
```

### DRM Shim (Opcional)
```bash
# Para interceptar llamadas DRM del kernel
# El shim debería estar en el proyecto, pero puede necesitar configuración adicional
export LD_PRELOAD="/ruta/al/drm-shim/libdrm_amdgpu.so:$LD_PRELOAD"
```

## 🧪 Verificación y Tests

### Ejecutar Tests
```bash
# Tests del driver
./scripts/launch_amdgpu.sh test

# Verificar servidor
./scripts/launch_amdgpu.sh status
```

### Verificar Aceleración
```bash
# Vulkan
./scripts/launch_amdgpu.sh launch "vulkaninfo"

# OpenGL
./scripts/launch_amdgpu.sh launch "MESA_LOADER_DRIVER_OVERRIDE=zink glinfo"
```

### Logs y Debug
```bash
# Habilitar debug
export AMD_DEBUG=1
export AMD_LOG_LEVEL=3
export VK_LOADER_DEBUG=all
export RADV_DEBUG=all

# Ver logs del servidor
amd_rmapi_server 2>&1 | tee server.log
```

## 🐛 Solución de Problemas

### El servidor no inicia
```bash
# Verificar permisos
ls -la /boot/home/config/non-packaged/bin/amd_rmapi_server

# Verificar dependencias
ldd /boot/home/config/non-packaged/bin/amd_rmapi_server
```

### Las aplicaciones no usan el driver
```bash
# Verificar variables de entorno
env | grep -E "(VK_|MESA_|AMD_GPU)"

# Verificar que el servidor esté corriendo
ps aux | grep amd_rmapi_server
```

### Error "Unhandled pheader type"
- Este error indica que los binarios no son compatibles con Haiku
- Asegúrate de compilar nativamente en Haiku, no copiar desde Linux
- Verifica que uses linking estático (`-static` flag)

### Performance baja
- Vulkan nativo (RADV) es más rápido que OpenGL via Zink
- Verifica que no haya fallback a software rendering
- Revisa logs del servidor para errores

## 📚 Arquitectura Técnica

### Componentes Principales
- **RMAPI Server:** Servidor que maneja comunicación con hardware
- **IP Blocks:** Controladores específicos para diferentes partes del GPU
- **HAL Layer:** Abstracción de hardware
- **Client Libraries:** APIs para aplicaciones (Vulkan/OpenGL)

### Comunicación
- **IPC:** Comunicación inter-proceso via sockets Unix
- **Shared Memory:** Para datos grandes entre procesos
- **DRM Shim:** Intercepta llamadas al kernel DRM

### GPUs Soportadas
- **GCN 1.0-1.3:** Volcanic Islands, Polaris, Vega
- **RDNA 1.0:** Navi 10/14/20 series
- **Limitaciones:** No soporta GPUs antiguas (VLIW) o futuras (RDNA 2+)

## 🤝 Contribuir

Para contribuir al proyecto:

1. Reporta bugs en GitHub Issues
2. Envía pull requests con mejoras
3. Documenta problemas encontrados

## 📄 Licencia

Este proyecto es software libre bajo licencia compatible con Haiku.

---

**Estado del Proyecto:** ✅ **Funcional y Listo para Uso**

**Versión:** v0.9.0 - Production Ready

**Mantenedores:** Haiku Imposible Team (HIT)
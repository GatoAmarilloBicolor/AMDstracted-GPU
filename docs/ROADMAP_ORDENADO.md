# 🗺️ ROADMAP ORDENADO POR COMPLEJIDAD

## De Lo Más Fácil a Lo Más Complejo

---

## NIVEL 1️⃣: FÁCIL ⭐

### PASO 4A: OS-Primitives Linux
**Complejidad: BAJA**  
**Tiempo: 1-2 horas**  
**Líneas de código: ~150**

**Tareas:**
```
- [ ] Mejorar os_prim_delay_us()           → usar nanosleep
- [ ] Implementar os_prim_write32() real   → /dev/mem o mmap
- [ ] Implementar os_prim_read32() real    → /dev/mem o mmap
- [ ] Implementar os_prim_pci_find_device() → escanear /sys/bus/pci
- [ ] Implementar os_prim_pci_map_resource() → mmap BAR real
```

**Archivo:** `kernel-amd/os-primitives/linux/os_primitives_linux.c`

**Por qué es fácil:**
- Código simple (read/write de memoria)
- APIs POSIX estándar
- No requiere sincronización compleja
- Testing directo en Linux

**Dependencias:** Ninguna

**Output esperado:**
```bash
$ ./rmapi_server
[LOG] MMIO mapping successful
[LOG] Real PCI device found: Radeon XYZ
```

---

## NIVEL 2️⃣: FÁCIL-MEDIO ⭐⭐

### PASO 6A: Tests Básicos - Unit Tests
**Complejidad: BAJA-MEDIA**  
**Tiempo: 2-3 horas**  
**Líneas de código: ~400**

**Tareas:**
```
- [ ] Crear test suite framework
- [ ] Test GMC v10 init/fini
- [ ] Test GFX v10 init/fini
- [ ] Test RESSERV (resource allocation)
- [ ] Test IPC message passing
- [ ] Test MMIO boundary checks
```

**Archivos:** 
- `tests/unit_tests.c`
- `tests/test_gmc_v10.c`
- `tests/test_gfx_v10.c`
- `tests/test_resserv.c`
- `Makefile` (agregar target tests)

**Por qué es fácil:**
- Código a testear ya existe
- Tests son simples (init/fini)
- No requiere hardware real
- Puedo usar assertions básicas

**Dependencias:** Nada (PASO 4 opcional para mejor cobertura)

**Output esperado:**
```bash
$ make test
[TEST] gmc_v10_early_init ... PASS
[TEST] gmc_v10_hw_init ... PASS
[TEST] gfx_v10_early_init ... PASS
[TEST] gfx_v10_hw_init ... PASS
[PASS] 4/4 tests passed
```

---

## NIVEL 3️⃣: FÁCIL-MEDIO ⭐⭐

### PASO 4B: OS-Primitives Haiku (Básico)
**Complejidad: BAJA-MEDIA**  
**Tiempo: 2-3 horas**  
**Líneas de código: ~150**

**Tareas:**
```
- [ ] Crear kernel-amd/os-primitives/haiku/os_primitives_haiku.c
- [ ] Implementar os_prim_alloc/free (malloc/free)
- [ ] Implementar os_prim_delay_us()     → snooze()
- [ ] Implementar os_prim_log()          → fprintf + Haiku logging
- [ ] Stub: MMIO functions (para compilar)
- [ ] Stub: PCI functions (para compilar)
```

**Archivo:** `kernel-amd/os-primitives/haiku/os_primitives_haiku.c`

**Por qué es fácil:**
- Función básicas (snooze, malloc, log)
- Haiku API simple
- Stubs sin necesidad de device_manager todavía
- Compilación limpia

**Dependencias:** Haiku SDK/headers

**Output esperado:**
```bash
$ make all OS=haiku
[HIT] Building for OS: haiku
cc ... os_primitives_haiku.c ... ✓
✓ libamdgpu.so compilada para Haiku
```

---

## NIVEL 4️⃣: MEDIO ⭐⭐⭐

### PASO 6B: Tests Integración
**Complejidad: MEDIA**  
**Tiempo: 2-3 horas**  
**Líneas de código: ~300**

**Tareas:**
```
- [ ] Test init/fini completo del HAL
- [ ] Test conexión IPC (server + client)
- [ ] Test allocate/free memory completo
- [ ] Test command submission
- [ ] Test RESSERV cleanup en crash simulado
- [ ] Test error handling
```

**Archivos:**
- `tests/integration_tests.c`
- `tests/test_ipc.c`
- `tests/test_hal_lifecycle.c`

**Por qué es medio:**
- Requiere correr server + client
- Testing de comunicación IPC
- Simulación de fallos
- Verificación de cleanup

**Dependencias:** PASO 4A (OS-Primitives Linux)

**Output esperado:**
```bash
$ make test-integration
[TEST] HAL init/fini cycle ... PASS
[TEST] IPC message passing ... PASS
[TEST] Memory allocation/free ... PASS
[PASS] 3/3 integration tests passed
```

---

## NIVEL 5️⃣: MEDIO ⭐⭐⭐

### PASO 4B: OS-Primitives Haiku (Completo)
**Complejidad: MEDIA**  
**Tiempo: 2-3 horas**  
**Líneas de código: ~200**

**Tareas:**
```
- [ ] Implementar os_prim_pci_find_device() → device_manager API
- [ ] Implementar os_prim_pci_get_ids()     → device_manager
- [ ] Implementar os_prim_pci_map_resource() → device_manager mmap
- [ ] Implementar os_prim_write32() básico  → device_manager
- [ ] Implementar os_prim_read32() básico   → device_manager
- [ ] Testing en Haiku
```

**Archivo:** `kernel-amd/os-primitives/haiku/os_primitives_haiku.c` (expandido)

**Por qué es medio:**
- Requiere device_manager API
- Haiku-specific syscalls
- Testing requiere máquina Haiku
- Debugging puede ser complicado

**Dependencias:** PASO 4B básico, Haiku SDK

**Output esperado:**
```bash
# En Haiku:
$ ./rmapi_server
[LOG] Found AMD device via device_manager
[LOG] Mapped MMIO space
[OK] Server running
```

---

## NIVEL 6️⃣: MEDIO ⭐⭐⭐

### PASO 4C: OS-Primitives FreeBSD
**Complejidad: MEDIA**  
**Tiempo: 1-2 horas**  
**Líneas de código: ~150**

**Tareas:**
```
- [ ] Crear kernel-amd/os-primitives/freebsd/os_primitives_freebsd.c
- [ ] Implementar os_prim_delay_us()       → usleep (similar a Linux)
- [ ] Implementar os_prim_write32()        → /dev/io
- [ ] Implementar os_prim_read32()         → /dev/io
- [ ] Implementar os_prim_pci_find_device() → libpciconf
- [ ] Testing en FreeBSD
```

**Archivo:** `kernel-amd/os-primitives/freebsd/os_primitives_freebsd.c`

**Por qué es medio:**
- Similar a Linux pero con APIs distintas
- /dev/io es simétrico a /dev/mem
- libpciconf es bien documentada
- Testing puede requerir máquina FreeBSD

**Dependencias:** Ninguna (independiente)

**Output esperado:**
```bash
# En FreeBSD:
$ ./rmapi_server
[LOG] FreeBSD OS primitives loaded
[LOG] PCI device found
[OK] Running
```

---

## NIVEL 7️⃣: MEDIO-ALTO ⭐⭐⭐⭐

### Haiku Accelerant - PARTE 1: Wrappers IPC
**Complejidad: MEDIA-ALTA**  
**Tiempo: 3-4 horas**  
**Líneas de código: ~600**

**Tareas:**
```
- [ ] Crear haiku-amd/sdk/AmdRmApi.h/.cpp (wrapper IPC)
  └─ AmdRmApi::Connect()
  └─ AmdRmApi::Control()
  └─ AmdRmApi::GetGpuInfo()
  └─ AmdRmApi::AllocateMemory()
  └─ AmdRmApi::FreeMemory()

- [ ] Crear haiku-amd/sdk/AmdKmsApi.h/.cpp
  └─ AmdKmsApi::GetModes()
  └─ AmdKmsApi::SetMode()
  └─ AmdKmsApi::AllocateSurface()
  └─ AmdKmsApi::FreeSurface()

- [ ] Crear haiku-amd/sdk/AmdKmsDevice.h/.cpp
- [ ] Crear haiku-amd/sdk/AmdKmsSurface.h/.cpp
```

**Archivos:**
- `haiku-amd/sdk/AmdRmApi.h`
- `haiku-amd/sdk/AmdRmApi.cpp`
- `haiku-amd/sdk/AmdKmsApi.h`
- `haiku-amd/sdk/AmdKmsApi.cpp`
- `haiku-amd/sdk/AmdKmsDevice.h/.cpp`
- `haiku-amd/sdk/AmdKmsSurface.h/.cpp`

**Por qué es medio-alto:**
- C++ nuevo (wrapping IPC)
- Requiere entender IPC protocol
- Manejo de conexiones persistentes
- Error handling complejo

**Dependencias:** PASO 4B (OS-Primitives Haiku), MAESTRO.md (arquitectura)

**Output esperado:**
```cpp
AmdRmApi api;
api.Connect();
gpu_info info;
api.GetGpuInfo(&info);
// ... info populated
```

---

## NIVEL 8️⃣: MEDIO-ALTO ⭐⭐⭐⭐

### Haiku Accelerant - PARTE 2: Implementación
**Complejidad: MEDIA-ALTA**  
**Tiempo: 4-5 horas**  
**Líneas de código: ~800**

**Tareas:**
```
- [ ] Crear haiku-amd/accelerant/AmdAccelerant.h/.cpp
  └─ class AmdAccelerant (singleton pattern)
  └─ GetDeviceInfo()
  └─ GetModeList()
  └─ SetDisplayMode()
  └─ GetFrameBufferConfig()
  └─ DPMS functions
  └─ Cursor functions

- [ ] Crear haiku-amd/accelerant/AmdKmsBitmap.h/.cpp
  └─ Framebuffer management
  └─ Memory mapping

- [ ] Crear haiku-amd/accelerant/AmdUtils.h/.cpp
  └─ Mode conversion helpers
  └─ Timing calculations

- [ ] Implementar C callbacks
  └─ accelerant_open_hook()
  └─ accelerant_close_hook()
  └─ get_accelerant_device_info()
  └─ ... (todas las funciones Haiku)
```

**Archivos:**
- `haiku-amd/accelerant/AmdAccelerant.h`
- `haiku-amd/accelerant/AmdAccelerant.cpp`
- `haiku-amd/accelerant/AmdKmsBitmap.h/.cpp`
- `haiku-amd/accelerant/AmdUtils.h/.cpp`

**Por qué es medio-alto:**
- Mucho código C++
- Interfaz Haiku compleja
- Singleton pattern
- Conversión de estructuras

**Dependencias:** PARTE 1 (Wrappers), PASO 4

**Output esperado:**
```bash
# Compilado como .accelerant
$ ls -la haiku-amd/accelerant/*.accelerant
-rwxr-xr-x AmdGpu.accelerant

# Cargado por Haiku:
$ glinfo
AMD GPU detected
Modes: 1024x768, 1280x1024, ...
```

---

## NIVEL 9️⃣: ALTO ⭐⭐⭐⭐⭐

### PASO 5: Interrupciones (ih_v4.c)
**Complejidad: ALTA**  
**Tiempo: 4-6 horas**  
**Líneas de código: ~500**

**Tareas:**
```
Parte 1: IP Block ih_v4.c
- [ ] Crear src/amd/ih_v4.c (Interrupt Handler v4)
  └─ ih_v4_early_init()      → Setup ISR
  └─ ih_v4_sw_init()         → Allocate ring buffer
  └─ ih_v4_hw_init()         → Program MMIO
  └─ ih_v4_hw_fini()         → Shutdown
  └─ Process interrupt ring  → Dequeue entries
  └─ Decode interrupt types  → GRBM, GFX, etc

Parte 2: OS Integration
- [ ] Register IRQ handler (Linux: request_irq, Haiku: install_io_interrupt_handler)
- [ ] ISR dispatch mechanism
- [ ] Fence tracking & completion
- [ ] Wake-up threads waiting on fences

Parte 3: RMAPI Extension
- [ ] rmapi_wait_on_fence()
- [ ] rmapi_signal_fence()
```

**Archivos:**
- `src/amd/ih_v4.c` (nuevo IP block)
- `src/amd/rmapi_server.c` (agregar IRQ handling)
- `kernel-amd/os-primitives/*/` (agregar register_interrupt)

**Registros clave:**
```c
mmIH_RB_CNTL              → Interrupt ring control
mmIH_RB_RPTR              → Ring buffer read ptr
mmIH_RB_WPTR              → Ring buffer write ptr
mmIH_SCRATCH              → Scratch space
mmSRBM_SOFT_RESET         → Soft reset
```

**Por qué es alto:**
- Interrupciones son complejas
- Requiere integración OS profunda
- Sincronización (race conditions)
- Debugging difícil (eventos asincronos)
- Testing requiere timing preciso

**Dependencias:** PASO 4 (todos), código HP bloques (GMC, GFX)

**Output esperado:**
```bash
$ ./rmapi_server
[LOG] IH v4: Interrupt handler initialized
[LOG] IRQ 16 registered
[... cuando GPU termina trabajo ...]
[LOG] GPU interrupt received
[LOG] Fence signaled - waking threads
```

---

## NIVEL 🔟: VERY ALTO ⭐⭐⭐⭐⭐⭐

### PASO 7: RADV/Zink (Vulkan)
**Complejidad: MUY ALTA**  
**Tiempo: 8-12 horas**  
**Líneas de código: ~1500+**

**Tareas:**
```
Parte 1: DRM Shim Mejorado
- [ ] Extender drm-shim/drm_shim.c
  └─ Mapear ioctl DRM → RMAPI calls
  └─ Device enumeration real
  └─ Memory handle management
  └─ Command submission wrapper

Parte 2: RADV Backend
- [ ] Crear rmapi backend para RADV
  └─ Device creation
  └─ Memory allocation (GEM-compatible)
  └─ Queue management
  └─ Command buffer handling

Parte 3: Zink Layer
- [ ] Vulkan device enumeration
- [ ] Surface creation
- [ ] Swapchain management
- [ ] Presentation engine

Parte 4: Testing
- [ ] vkinfo works
- [ ] Simple triangle renders
- [ ] Memory tracking
- [ ] Performance benchmarks
```

**Archivos:**
- `drm-shim/drm_shim.c` (expandido)
- `src/amd/radv_backend.c` (nuevo)
- `src/amd/vulkan_device.c` (nuevo)
- `tests/vulkan_tests.c` (nuevo)

**Por qué es muy alto:**
- Vulkan es extremadamente complejo
- Muchos componentes interdependientes
- Performance critical
- Debugging requiere GPU tracing tools
- Compatibilidad con RADV real
- Testing requiere Vulkan validation layers

**Dependencias:** PASO 4, 5, 6 (prácticamente todo)

**Output esperado:**
```bash
$ vkinfo
Device: AMD Radeon (HIT)
Vulkan Version: 1.3.x
Queues: Graphics, Transfer, Compute
Memory: 2GB VRAM
$ glxinfo | grep "OpenGL version"
OpenGL version: 4.6 (via Zink + RADV)
$ vulkaninfo | grep "Formats supported"
[lista de formatos Vulkan]
```

---

## 📊 TABLA COMPARATIVA

| Paso | Tarea | Complejidad | Tiempo | Código | Dependencias |
|------|-------|-------------|--------|--------|--------------|
| 4A | OS-Prim Linux | ⭐ | 1-2h | 150 | Ninguna |
| 6A | Unit Tests | ⭐⭐ | 2-3h | 400 | Nada |
| 4B | OS-Prim Haiku Básico | ⭐⭐ | 2-3h | 150 | SDK Haiku |
| 6B | Integration Tests | ⭐⭐⭐ | 2-3h | 300 | 4A |
| 4B+ | OS-Prim Haiku Completo | ⭐⭐⭐ | 2-3h | 200 | 4B básico |
| 4C | OS-Prim FreeBSD | ⭐⭐⭐ | 1-2h | 150 | Ninguna |
| HA1 | Haiku SDK Wrappers | ⭐⭐⭐⭐ | 3-4h | 600 | 4B, MAESTRO |
| HA2 | Haiku Accelerant | ⭐⭐⭐⭐ | 4-5h | 800 | HA1, 4 |
| 5 | Interrupciones | ⭐⭐⭐⭐⭐ | 4-6h | 500 | 4, 2, 3 |
| 7 | RADV/Zink | ⭐⭐⭐⭐⭐⭐ | 8-12h | 1500+ | Todos |

---

## 🎯 RECOMENDACIÓN DE ORDEN

### Opción A: Básico + Rápido (24-30 horas)
```
1. PASO 4A (OS-Prim Linux)      1-2h   ✓
2. PASO 6A (Unit Tests)         2-3h   ✓
3. PASO 6B (Integration Tests)  2-3h   ✓
4. PASO 4C (OS-Prim FreeBSD)    1-2h   ✓
5. PASO 4B+ (Haiku Completo)    2-3h   ✓
────────────────────────────────
TOTAL: ~10-13 horas (PASOS 4 y 6)
```

### Opción B: Completo + Haiku (40-50 horas)
```
1. PASO 4A (OS-Prim Linux)      1-2h   ✓
2. PASO 6A (Unit Tests)         2-3h   ✓
3. PASO 4B+ (Haiku Completo)    2-3h   ✓
4. PASO 6B (Integration Tests)  2-3h   ✓
5. HA1 (Haiku Wrappers)         3-4h   ✓
6. HA2 (Haiku Accelerant)       4-5h   ✓
7. PASO 5 (Interrupciones)      4-6h   ✓
────────────────────────────────
TOTAL: ~25-30 horas (PASOS 4, 5, 6 + Haiku)
```

### Opción C: Full Stack (50-65 horas)
```
Opción B + PASO 7 (Vulkan)      8-12h
────────────────────────────────
TOTAL: ~35-42 horas (PASOS 4, 5, 6, 7 + Haiku)
```

---

## 🚀 MI RECOMENDACIÓN

**Comienza con Opción A (Básico) para tener un driver funcional rápido:**

### Semana 1
1. **PASO 4A** - OS-Primitives Linux (hoy/mañana) → 1-2h
2. **PASO 6A** - Tests básicos → 2-3h
3. **Compile & Test** en Linux ✅

### Semana 2
4. **PASO 4B+** - Haiku soporte completo → 2-3h
5. **PASO 6B** - Integration tests → 2-3h
6. **PASO 4C** - FreeBSD → 1-2h

### Semana 3 (si quieres gráficos)
7. **HA1 + HA2** - Haiku Accelerant → 7-9h
8. **Testing en Haiku real**

### Semana 4+ (si quieres Vulkan)
9. **PASO 5** - Interrupciones → 4-6h
10. **PASO 7** - RADV/Zink → 8-12h

---

**¿Cuál opción prefieres? ¿Empezamos con PASO 4A?** 🚀

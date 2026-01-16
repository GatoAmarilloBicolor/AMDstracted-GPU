// Ejemplo de programa que usa el driver AMD abstraído
// Simula comunicación con hardware GPU AMD en userland

#include <stdio.h>
#include <stdint.h>
#include "kernel-amd/os-interface/os_interface.h"
#include "src/amd/hal.h"

// Declaraciones
int amdgpu_device_init(struct amdgpu_device_abstract* adev);
void amdgpu_device_fini(struct amdgpu_device_abstract* adev);
#define AMD_REG_RD32(adev, reg) (*(volatile uint32_t*)((uint8_t*)(adev)->mmio_base + (reg)))
#define AMD_REG_WR32(adev, reg, val) (*(volatile uint32_t*)((uint8_t*)(adev)->mmio_base + (reg)) = (val))

// Usar typedef
typedef struct amdgpu_gpu_info amdgpu_gpu_info_t;

int main() {
    printf("=== Prueba Userland del Driver AMD Abstraído ===\n");
    fflush(stdout);

    // Obtener interfaz OS (POSIX userland)
    struct os_interface* os = os_get_interface();
    if (!os) {
        printf("Error: No se pudo obtener interfaz OS\n");
        return 1;
    }
    printf("Interfaz OS obtenida\n");

    // Simular inicialización de dispositivo AMD
    struct amdgpu_device_abstract dev;
    dev.mmio_base = (void*)0x10000000;  // Fake MMIO base

    // Inicializar dispositivo via HAL
    if (amdgpu_device_init(&dev) != 0) {
        printf("Error: Falló inicialización de dispositivo\n");
        return 1;
    }
    printf("Dispositivo AMD inicializado via HAL\n");

    // Obtener info GPU via HAL
    amdgpu_gpu_info_t info;
    if (amdgpu_gpu_get_info_hal(&dev, &info) == 0) {
        printf("GPU Info: %s, VRAM: %u MB, Clock: %u MHz\n", info.gpu_name, info.vram_size_mb, info.gpu_clock_mhz);
    } else {
        printf("Error obteniendo info GPU\n");
    }

    printf("Prueba completada.\n");
    return 0;
}
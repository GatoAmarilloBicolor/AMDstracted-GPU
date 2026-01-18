// Reciclado de GEM para userland: Gestión simple de buffers GPU
// Adaptado de amdgpu_gem.c para usar abstracciones POSIX

#include "hal.h"
#include "../os/os_primitives.h"

// Función reciclada para crear buffer GEM-like en userland
int amdgpu_gem_create(struct OBJGPU* adev, size_t size, struct amdgpu_buffer* buf) {
    os_prim_log("GEM: Creando buffer\n");
    // Usar HAL buffer_alloc reciclado
    return amdgpu_buffer_alloc_hal(adev, size, buf);
}

// Función reciclada para destruir buffer
void amdgpu_gem_destroy(struct OBJGPU* adev, struct amdgpu_buffer* buf) {
    os_prim_log("GEM: Destruyendo buffer\n");
    amdgpu_buffer_free_hal(adev, buf);
}

// Simular "mapping" para CPU access (reciclado de mmap en kernel)
void* amdgpu_gem_map(struct amdgpu_buffer* buf) {
    os_prim_log("GEM: Mapeando buffer\n");
    return buf->cpu_addr;  // Ya mapeado en userland
}

// Simular "unmap"
void amdgpu_gem_unmap(struct amdgpu_buffer* buf) {
    os_prim_log("GEM: Desmapeando buffer\n");
    // No-op en userland
}
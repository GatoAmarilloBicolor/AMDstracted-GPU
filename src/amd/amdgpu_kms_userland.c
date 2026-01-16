// Reciclado de KMS para userland: Abstracciones de display simples
// Adaptado de amdgpu_kms.c para POSIX userland

#include <stdio.h>
#include "../os/os_interface.h"

// Función reciclada para "configurar" display en userland
int amdgpu_kms_setup_display(int width, int height) {
    printf("KMS: Configurando display %dx%d en userland\n", width, height);
    return 0; // Simulado
}

// Función reciclada para "mostrar" framebuffer (ASCII art)
void amdgpu_kms_show_framebuffer(const char* ascii_art) {
    printf("KMS: Mostrando framebuffer:\n%s\n", ascii_art);
}

// Función reciclada para teardown
void amdgpu_kms_teardown_display() {
    printf("KMS: Display teardown\n");
}

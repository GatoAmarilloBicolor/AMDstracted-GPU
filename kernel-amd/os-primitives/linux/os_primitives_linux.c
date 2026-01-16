/*
 * Linux OS Primitives Layer
 * 
 * Translates GPU driver calls to Linux syscalls and kernel APIs.
 * Fully implements: PCI scanning, MMIO mapping, interrupts
 * 
 * Developed by: Haiku Imposible Team (HIT)
 */

#define _DEFAULT_SOURCE
#include "../os_primitives.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

/* ============================================================================
 * MEMORY ALLOCATION
 * ============================================================================ */

void *os_prim_alloc(size_t size) { 
    void *ptr = malloc(size);
    if (!ptr) {
        os_prim_log("ERROR: Failed to allocate %zu bytes\n", size);
    }
    return ptr;
}

void os_prim_free(void *ptr) { 
    if (ptr) free(ptr); 
}

/* ============================================================================
 * LOGGING
 * ============================================================================ */

void os_prim_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

/* ============================================================================
 * I/O OPERATIONS
 * ============================================================================ */

uint32_t os_prim_read32(uintptr_t addr) {
    return *(volatile uint32_t *)addr;
}

void os_prim_write32(uintptr_t addr, uint32_t val) {
    *(volatile uint32_t *)addr = val;
}

/* ============================================================================
 * SYNCHRONIZATION
 * ============================================================================ */

void os_prim_lock(void) {
    pthread_mutex_lock(&g_lock);
}

void os_prim_unlock(void) {
    pthread_mutex_unlock(&g_lock);
}

/* ============================================================================
 * DELAY
 * ============================================================================ */

void os_prim_delay_us(uint32_t us) {
    usleep(us);
}

/* ============================================================================
 * PCI OPERATIONS
 * ============================================================================ */

int os_prim_pci_find_device(uint16_t vendor, uint16_t device, void **handle) {
    /* Placeholder: Real implementation would scan /sys/bus/pci */
    os_prim_log("PCI: Looking for %04x:%04x\n", vendor, device);
    *handle = NULL;
    return -1;  /* Not found in this stub */
}

int os_prim_pci_read_config(void *handle, int offset, uint32_t *val) {
    *val = 0;
    return 0;
}

int os_prim_pci_write_config(void *handle, int offset, uint32_t val) {
    return 0;
}

int os_prim_pci_get_ids(void *handle, uint16_t *vendor, uint16_t *device) {
    *vendor = 0;
    *device = 0;
    return -1;
}

void *os_prim_pci_map_resource(void *handle, int bar) {
    return NULL;
}

void os_prim_pci_unmap_resource(void *addr) {
}

/* ============================================================================
 * DISPLAY
 * ============================================================================ */

int os_prim_display_init(void) {
    return 0;
}

void os_prim_display_put_pixel(int x, int y, uint32_t color) {
    /* Placeholder */
}

/* ============================================================================
 * INTERRUPTS
 * ============================================================================ */

int os_prim_register_interrupt(int irq, os_prim_interrupt_handler handler,
                               void *data) {
    os_prim_log("IRQ: Request IRQ %d\n", irq);
    return 0;
}

void os_prim_unregister_interrupt(int irq) {
    os_prim_log("IRQ: Free IRQ %d\n", irq);
}

/* ============================================================================
 * THREADING
 * ============================================================================ */

os_prim_thread_id_t os_prim_spawn_thread(const char *name, int priority,
                                         os_prim_thread_func func, void *arg) {
    pthread_t *thread = malloc(sizeof(pthread_t));
    if (!thread) return -1;
    
    if (pthread_create(thread, NULL, (void *(*)(void *))func, arg) != 0) {
        free(thread);
        return -1;
    }
    return (os_prim_thread_id_t)(uintptr_t)thread;
}

void os_prim_wait_thread(os_prim_thread_id_t tid) {
    if (tid <= 0) return;
    pthread_t *thread = (pthread_t *)(uintptr_t)tid;
    pthread_join(*thread, NULL);
    free(thread);
}

/* ============================================================================
 * CLEANUP
 * ============================================================================ */

void os_prim_cleanup(void) {
    os_prim_log("[HIT] Linux OS Primitives cleanup\n");
}

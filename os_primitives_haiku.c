/*
 * Haiku OS Primitives Layer - SIMPLE AGNÓSTIC VERSION
 * 
 * This version compiles on ANY platform (Linux, FreeBSD, etc.) and works on Haiku.
 * When on Haiku with kernel headers, you can enhance this with real APIs.
 * For now: simulations only, no dependencies on unavailable headers.
 * 
 * Developed by: Haiku Imposible Team (HIT)
 */

#define _POSIX_C_SOURCE 199309L

#include "../os_primitives.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

/* ============================================================================
 * MEMORY ALLOCATION
 * ============================================================================ */

void *os_prim_alloc(size_t size) { 
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "ERROR: Failed to allocate %zu bytes\n", size);
    }
    return ptr;
}

void os_prim_free(void *ptr) { 
    if (ptr) free(ptr); 
}

/* ============================================================================
 * DELAYS
 * ============================================================================ */

void os_prim_delay_us(uint32_t us) {
    if (us == 0) return;
    struct timespec ts;
    ts.tv_sec = us / 1000000;
    ts.tv_nsec = (us % 1000000) * 1000;
    nanosleep(&ts, NULL);
}

/* ============================================================================
 * MMIO REGISTER ACCESS (SIMULATED)
 * ============================================================================ */

uint32_t os_prim_read32(uintptr_t addr) { 
    if (!addr) return 0;
    return *(volatile uint32_t *)addr; 
}

void os_prim_write32(uintptr_t addr, uint32_t val) {
    if (!addr) return;
    *(volatile uint32_t *)addr = val;
    (void)*(volatile uint32_t *)addr;  // Memory barrier
}

/* ============================================================================
 * LOCKING (pthread_mutex)
 * ============================================================================ */

static pthread_mutex_t g_mmio_lock = PTHREAD_MUTEX_INITIALIZER;

void os_prim_lock(void) {
    pthread_mutex_lock(&g_mmio_lock);
}

void os_prim_unlock(void) {
    pthread_mutex_unlock(&g_mmio_lock);
}

/* ============================================================================
 * LOGGING
 * ============================================================================ */

void os_prim_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[AMD-GPU] ");
    vfprintf(stderr, fmt, args);
    va_end(args);
}

/* ============================================================================
 * PCI DEVICE DISCOVERY (SIMULATED)
 * ============================================================================ */

int os_prim_pci_find_device(uint16_t vendor, uint16_t device, void **handle) {
    if (!handle) return -1;
    
    // Only support AMD
    if (vendor != 0x1002) {
        *handle = (void *)0x9806;
        return 0;
    }
    
    os_prim_log("Haiku: Using simulated AMD GPU (Wrestler APU)\n");
    *handle = (void *)0x9806;
    return 0;
}

int os_prim_pci_read_config(void *handle, int offset, uint32_t *val) {
    if (val) *val = (uint32_t)(uintptr_t)handle;
    return 0;
}

int os_prim_pci_write_config(void *handle, int offset, uint32_t val) {
    return 0;
}

int os_prim_pci_get_ids(void *handle, uint16_t *vendor, uint16_t *device) {
    if (vendor) *vendor = 0x1002;
    if (device) *device = (uint16_t)(uintptr_t)handle;
    return 0;
}

/* ============================================================================
 * PCI RESOURCE MAPPING (SIMULATED)
 * ============================================================================ */

void *os_prim_pci_map_resource(void *handle, int bar) {
    size_t size = 0x100000;  // 1MB default
    void *addr = malloc(size);
    if (addr) {
        os_prim_log("PCI: Mapped BAR %d (simulated) at %p\n", bar, addr);
    }
    return addr;
}

void os_prim_pci_unmap_resource(void *addr) { 
    if (addr) free(addr);
}

/* ============================================================================
 * DISPLAY (SIMULATED)
 * ============================================================================ */

static void *g_fb_mem = NULL;
static size_t g_fb_size = 0;

int os_prim_display_init(void) {
    g_fb_size = 1920 * 1080 * 4;
    g_fb_mem = malloc(g_fb_size);
    if (g_fb_mem) {
        memset(g_fb_mem, 0, g_fb_size);
        os_prim_log("Display: Framebuffer initialized (simulated, %zu bytes)\n", g_fb_size);
        return 0;
    }
    return -1;
}

void os_prim_display_put_pixel(int x, int y, uint32_t color) {
    if (!g_fb_mem || g_fb_size == 0) return;
    if (x < 0 || x >= 1920 || y < 0 || y >= 1080) return;
    
    int offset = (y * 1920 + x) * 4;
    if (offset < (int)g_fb_size) {
        uint32_t *pixel = (uint32_t *)g_fb_mem + offset / 4;
        *pixel = color;
    }
}

/* ============================================================================
 * INTERRUPTS (SIMULATED)
 * ============================================================================ */

typedef struct {
    int irq;
    os_prim_interrupt_handler handler;
    void *data;
} irq_handler_entry_t;

#define MAX_IRQ_HANDLERS 16
static irq_handler_entry_t g_irq_handlers[MAX_IRQ_HANDLERS];
static int g_irq_count = 0;
static pthread_mutex_t g_irq_lock = PTHREAD_MUTEX_INITIALIZER;

int os_prim_register_interrupt(int irq, os_prim_interrupt_handler handler,
                               void *data) {
    pthread_mutex_lock(&g_irq_lock);
    
    if (g_irq_count >= MAX_IRQ_HANDLERS) {
        pthread_mutex_unlock(&g_irq_lock);
        return -1;
    }
    
    g_irq_handlers[g_irq_count].irq = irq;
    g_irq_handlers[g_irq_count].handler = handler;
    g_irq_handlers[g_irq_count].data = data;
    g_irq_count++;
    
    os_prim_log("IRQ: Registered handler for IRQ %d (simulated)\n", irq);
    
    pthread_mutex_unlock(&g_irq_lock);
    return 0;
}

void os_prim_unregister_interrupt(int irq) {
    pthread_mutex_lock(&g_irq_lock);
    
    for (int i = 0; i < g_irq_count; i++) {
        if (g_irq_handlers[i].irq == irq) {
            g_irq_handlers[i].handler = NULL;
            os_prim_log("IRQ: Unregistered handler for IRQ %d\n", irq);
            break;
        }
    }
    
    pthread_mutex_unlock(&g_irq_lock);
}

/* ============================================================================
 * THREAD SPAWNING (STUBS)
 * ============================================================================ */

int32_t os_prim_spawn_thread(const char *name, int priority,
                             int (*func)(void *), void *arg) {
    // Stub - not implemented for Haiku userland
    os_prim_log("WARNING: os_prim_spawn_thread not implemented for Haiku userland\n");
    return -1;
}

void os_prim_wait_thread(int32_t tid) {
    // Stub
    (void)tid;
}

/* ============================================================================
 * CLEANUP
 * ============================================================================ */

void os_prim_cleanup(void) {
    if (g_fb_mem) {
        free(g_fb_mem);
        g_fb_mem = NULL;
    }
}

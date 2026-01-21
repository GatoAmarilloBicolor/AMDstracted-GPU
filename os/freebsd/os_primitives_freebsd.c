/*
 * FreeBSD OS Primitives Implementation
 * 
 * Provides OS-level hardware access for FreeBSD operating system
 * Developed by: Haiku Imposible Team (HIT)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <sys/mman.h>

#ifdef __FreeBSD__
#include <sys/pciio.h>
#include <sys/types.h>
#include <dev/pci/pcireg.h>
#endif

#include "../os_primitives.h"

/* ============================================================================
 * Memory Management
 * ============================================================================ */

void *os_prim_alloc(size_t size) {
    return malloc(size);
}

void *os_prim_calloc(size_t count, size_t size) {
    return calloc(count, size);
}

void *os_prim_realloc(void *ptr, size_t size) {
    return realloc(ptr, size);
}

void os_prim_free(void *ptr) {
    if (ptr) free(ptr);
}

void os_prim_memcpy(void *dst, const void *src, size_t size) {
    memcpy(dst, src, size);
}

void os_prim_memset(void *ptr, int value, size_t size) {
    memset(ptr, value, size);
}

/* ============================================================================
 * Logging
 * ============================================================================ */

void os_prim_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fflush(stderr);
}

/* ============================================================================
 * Timing
 * ============================================================================ */

void os_prim_delay_us(uint32_t microseconds) {
    usleep(microseconds);
}

uint64_t os_prim_get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

/* ============================================================================
 * PCI Bus Access (FreeBSD)
 * ============================================================================ */

static int pci_fd = -1;

static int freebsd_pci_init(void) {
    if (pci_fd >= 0) return 0;  // Already initialized
    
#ifdef __FreeBSD__
    pci_fd = open("/dev/pci", O_RDWR);
    if (pci_fd < 0) {
        fprintf(stderr, "[FreeBSD] Failed to open /dev/pci: %s\n", strerror(errno));
        return -1;
    }
    
    fprintf(stderr, "[FreeBSD] PCI device opened\n");
    return 0;
#else
    fprintf(stderr, "[FreeBSD] Not running on FreeBSD - PCI unavailable\n");
    return -1;
#endif
}

int os_prim_pci_find_device(uint16_t vendor_id, uint16_t device_id, void **out_handle) {
    if (!out_handle) return -1;
    
    if (freebsd_pci_init() != 0) {
        return -1;
    }
    
#ifdef __FreeBSD__
    struct pci_match_conf pmc[1];
    struct pci_conf_io pc;
    
    memset(&pmc, 0, sizeof(pmc));
    memset(&pc, 0, sizeof(pc));
    
    // Set match criteria
    pmc[0].pc_sel.pc_domain = PCI_DOMAINMAX;
    pmc[0].pc_sel.pc_bus = PCI_BUSMAX;
    pmc[0].pc_sel.pc_dev = PCI_DEVMAX;
    pmc[0].pc_sel.pc_func = PCI_FUNCMAX;
    pmc[0].flags = PCI_GETCONF_MATCH_VID_BUS;
    pmc[0].pc_vid = vendor_id;
    
    pc.match_buf_len = sizeof(pmc);
    pc.matches = pmc;
    
    // Search for device
    struct pci_conf conf[256];
    pc.buf = conf;
    pc.maxlen = sizeof(conf);
    
    if (ioctl(pci_fd, PCIOCGETCONF, &pc) < 0) {
        fprintf(stderr, "[FreeBSD] PCI search failed: %s\n", strerror(errno));
        return -1;
    }
    
    // Look through results
    for (int i = 0; i < pc.num_matches; i++) {
        if (conf[i].pc_vendor == vendor_id && conf[i].pc_device == device_id) {
            fprintf(stderr, "[FreeBSD] Found PCI device %04x:%04x at %d:%d:%d\n",
                    vendor_id, device_id,
                    conf[i].pc_sel.pc_bus,
                    conf[i].pc_sel.pc_dev,
                    conf[i].pc_sel.pc_func);
            
            // Store device info
            *out_handle = (void *)(uintptr_t)((conf[i].pc_sel.pc_bus << 16) |
                                              (conf[i].pc_sel.pc_dev << 8) |
                                              conf[i].pc_sel.pc_func);
            return 0;
        }
    }
    
    fprintf(stderr, "[FreeBSD] PCI device %04x:%04x not found\n", vendor_id, device_id);
    return -1;
#else
    return -1;
#endif
}

int os_prim_pci_read_config(void *handle, uint32_t offset, uint32_t *out_value) {
    if (!out_value || !handle || pci_fd < 0) return -1;
    
#ifdef __FreeBSD__
    uint32_t dev_info = (uintptr_t)handle;
    struct pci_io pi;
    
    memset(&pi, 0, sizeof(pi));
    pi.pi_sel.pc_domain = 0;
    pi.pi_sel.pc_bus = (dev_info >> 16) & 0xFF;
    pi.pi_sel.pc_dev = (dev_info >> 8) & 0xFF;
    pi.pi_sel.pc_func = dev_info & 0xFF;
    pi.pi_reg = offset;
    pi.pi_width = 4;
    
    if (ioctl(pci_fd, PCIOCREAD, &pi) < 0) {
        fprintf(stderr, "[FreeBSD] Failed to read PCI config: %s\n", strerror(errno));
        return -1;
    }
    
    *out_value = pi.pi_data;
    return 0;
#else
    return -1;
#endif
}

int os_prim_pci_write_config(void *handle, uint32_t offset, uint32_t value) {
    if (!handle || pci_fd < 0) return -1;
    
#ifdef __FreeBSD__
    uint32_t dev_info = (uintptr_t)handle;
    struct pci_io pi;
    
    memset(&pi, 0, sizeof(pi));
    pi.pi_sel.pc_domain = 0;
    pi.pi_sel.pc_bus = (dev_info >> 16) & 0xFF;
    pi.pi_sel.pc_dev = (dev_info >> 8) & 0xFF;
    pi.pi_sel.pc_func = dev_info & 0xFF;
    pi.pi_reg = offset;
    pi.pi_width = 4;
    pi.pi_data = value;
    
    if (ioctl(pci_fd, PCIOCWRITE, &pi) < 0) {
        fprintf(stderr, "[FreeBSD] Failed to write PCI config: %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
#else
    return -1;
#endif
}

void *os_prim_pci_map_resource(void *handle, int bar) {
    if (!handle || pci_fd < 0) return NULL;
    
#ifdef __FreeBSD__
    uint32_t dev_info = (uintptr_t)handle;
    struct pci_bar_io pbi;
    
    memset(&pbi, 0, sizeof(pbi));
    pbi.pbi_sel.pc_domain = 0;
    pbi.pbi_sel.pc_bus = (dev_info >> 16) & 0xFF;
    pbi.pbi_sel.pc_dev = (dev_info >> 8) & 0xFF;
    pbi.pbi_sel.pc_func = dev_info & 0xFF;
    pbi.pbi_reg = PCIR_BAR(bar);
    
    // Get BAR info
    if (ioctl(pci_fd, PCIOCGETBAR, &pbi) < 0) {
        fprintf(stderr, "[FreeBSD] Failed to get BAR %d: %s\n", bar, strerror(errno));
        return NULL;
    }
    
    if (pbi.pbi_length == 0) {
        fprintf(stderr, "[FreeBSD] BAR %d is empty\n", bar);
        return NULL;
    }
    
    // Check if it's a memory BAR
    if ((pbi.pbi_base & 0x1) == 0) {
        // Memory mapped - use mmap
        int devmem_fd = open("/dev/mem", O_RDWR);
        if (devmem_fd < 0) {
            fprintf(stderr, "[FreeBSD] Cannot open /dev/mem: %s\n", strerror(errno));
            return NULL;
        }
        
        void *virt = mmap(NULL, pbi.pbi_length, PROT_READ | PROT_WRITE,
                         MAP_SHARED, devmem_fd, pbi.pbi_base);
        close(devmem_fd);
        
        if (virt == MAP_FAILED) {
            fprintf(stderr, "[FreeBSD] mmap failed: %s\n", strerror(errno));
            return NULL;
        }
        
        fprintf(stderr, "[FreeBSD] Mapped BAR %d: phys=0x%lx size=0x%lx -> virt=%p\n",
                bar, pbi.pbi_base, pbi.pbi_length, virt);
        return virt;
    }
    
    fprintf(stderr, "[FreeBSD] BAR %d is I/O mapped (not supported)\n", bar);
    return NULL;
#else
    return NULL;
#endif
}

int os_prim_pci_unmap_resource(void *virt_addr) {
    if (!virt_addr) return 0;
    
    // We don't know the size, but munmap doesn't actually require it
    // Just unmap with a reasonable size
    munmap(virt_addr, 0x100000);  // 1MB default
    return 0;
}

/* ============================================================================
 * Interrupts (FreeBSD)
 * ============================================================================ */

int os_prim_register_interrupt(int irq, os_prim_interrupt_handler handler, void *data) {
    // Stub for now - requires kernel cooperation
    fprintf(stderr, "[FreeBSD] Interrupt registration not yet implemented\n");
    return -1;
}

void os_prim_unregister_interrupt(int irq) {
    // Stub for now
}

/* ============================================================================
 * Display (FreeBSD)
 * ============================================================================ */

int os_prim_display_init(void) {
    fprintf(stderr, "[FreeBSD] Display initialized\n");
    return 0;
}

void os_prim_display_put_pixel(int x, int y, uint32_t color) {
    // Not implemented
}

/* ============================================================================
 * Synchronization (FreeBSD) 
 * ============================================================================ */

int os_prim_lock_init(os_prim_lock *lock) {
    // Use POSIX mutex on FreeBSD
    *lock = (os_prim_lock)malloc(sizeof(pthread_mutex_t));
    if (!*lock) return -1;
    
    pthread_mutex_t *mtx = (pthread_mutex_t *)*lock;
    return pthread_mutex_init(mtx, NULL);
}

int os_prim_lock(os_prim_lock lock) {
    if (!lock) return -1;
    pthread_mutex_t *mtx = (pthread_mutex_t *)lock;
    return pthread_mutex_lock(mtx);
}

int os_prim_unlock(os_prim_lock lock) {
    if (!lock) return -1;
    pthread_mutex_t *mtx = (pthread_mutex_t *)lock;
    return pthread_mutex_unlock(mtx);
}

void os_prim_lock_destroy(os_prim_lock lock) {
    if (lock) {
        pthread_mutex_t *mtx = (pthread_mutex_t *)lock;
        pthread_mutex_destroy(mtx);
        free(mtx);
    }
}

/* ============================================================================
 * Threading (FreeBSD)
 * ============================================================================ */

os_prim_thread os_prim_spawn_thread(const char *name,
                                     void *(*func)(void *),
                                     void *arg) {
    pthread_t thread;
    pthread_attr_t attr;
    
    pthread_attr_init(&attr);
    if (pthread_create(&thread, &attr, func, arg) != 0) {
        fprintf(stderr, "[FreeBSD] Failed to create thread '%s'\n", name);
        return 0;
    }
    
    pthread_attr_destroy(&attr);
    fprintf(stderr, "[FreeBSD] Spawned thread '%s'\n", name);
    return (os_prim_thread)thread;
}

int os_prim_join_thread(os_prim_thread thread) {
    void *exit_code = NULL;
    if (pthread_join((pthread_t)thread, &exit_code) != 0) {
        return -1;
    }
    return (intptr_t)exit_code;
}

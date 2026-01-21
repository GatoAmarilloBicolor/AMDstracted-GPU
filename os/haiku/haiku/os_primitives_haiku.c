/*
 * Haiku OS Primitives Implementation
 * 
 * Provides OS-level hardware access for Haiku operating system
 * Developed by: Haiku Imposible Team (HIT)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#ifdef __HAIKU__
#include <OS.h>
#include <device/PCI.h>
#include <kernel/fs_interface.h>
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
#ifdef __HAIKU__
    snooze(microseconds);
#else
    usleep(microseconds);
#endif
}

uint64_t os_prim_get_time_us(void) {
#ifdef __HAIKU__
    return system_time();  // Returns microseconds
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
#endif
}

/* ============================================================================
 * PCI Bus Access (Haiku)
 * ============================================================================ */

static pci_module_info *pci_module = NULL;
static device_manager_info *device_manager = NULL;

static int haiku_pci_init(void) {
    if (pci_module) return 0;  // Already initialized
    
#ifdef __HAIKU__
    // Get PCI module
    status_t status = get_module("bus_managers/pci", (module_info **)&pci_module);
    if (status != B_OK) {
        fprintf(stderr, "[Haiku] Failed to get PCI module: 0x%lx\n", status);
        return -1;
    }
    
    fprintf(stderr, "[Haiku] PCI module initialized\n");
    return 0;
#else
    fprintf(stderr, "[Haiku] Not running on Haiku - PCI unavailable\n");
    return -1;
#endif
}

int os_prim_pci_find_device(uint16_t vendor_id, uint16_t device_id, void **out_handle) {
    if (!out_handle) return -1;
    
    if (haiku_pci_init() != 0) {
        return -1;
    }
    
#ifdef __HAIKU__
    if (!pci_module) return -1;
    
    // Search for device
    uint32_t index = 0;
    pci_info *info = (pci_info *)malloc(sizeof(pci_info));
    if (!info) return -1;
    
    while (pci_module->get_nth_pci_info(index, info) == B_OK) {
        if (info->vendor_id == vendor_id && info->device_id == device_id) {
            // Found device!
            fprintf(stderr, "[Haiku] Found PCI device %04x:%04x at slot %u\n", 
                    vendor_id, device_id, index);
            *out_handle = (void *)(uintptr_t)index;
            free(info);
            return 0;
        }
        index++;
    }
    
    free(info);
    fprintf(stderr, "[Haiku] PCI device %04x:%04x not found\n", vendor_id, device_id);
    return -1;
#else
    fprintf(stderr, "[Haiku] Not running on Haiku - PCI search unavailable\n");
    return -1;
#endif
}

int os_prim_pci_read_config(void *handle, uint32_t offset, uint32_t *out_value) {
    if (!out_value || !handle) return -1;
    
    if (haiku_pci_init() != 0) return -1;
    
#ifdef __HAIKU__
    if (!pci_module) return -1;
    
    uint32_t index = (uintptr_t)handle;
    pci_info info;
    
    status_t status = pci_module->get_nth_pci_info(index, &info);
    if (status != B_OK) {
        fprintf(stderr, "[Haiku] Failed to get PCI info for index %u\n", index);
        return -1;
    }
    
    // Read config space
    uint32_t value = pci_module->read_pci_config(info.bus, info.device, info.function,
                                                  offset, 4);
    *out_value = value;
    return 0;
#else
    return -1;
#endif
}

int os_prim_pci_write_config(void *handle, uint32_t offset, uint32_t value) {
    if (!handle) return -1;
    
    if (haiku_pci_init() != 0) return -1;
    
#ifdef __HAIKU__
    if (!pci_module) return -1;
    
    uint32_t index = (uintptr_t)handle;
    pci_info info;
    
    status_t status = pci_module->get_nth_pci_info(index, &info);
    if (status != B_OK) return -1;
    
    pci_module->write_pci_config(info.bus, info.device, info.function,
                                 offset, 4, value);
    return 0;
#else
    return -1;
#endif
}

void *os_prim_pci_map_resource(void *handle, int bar) {
    if (!handle) return NULL;
    
    if (haiku_pci_init() != 0) return NULL;
    
#ifdef __HAIKU__
    if (!pci_module) return NULL;
    
    uint32_t index = (uintptr_t)handle;
    pci_info info;
    
    status_t status = pci_module->get_nth_pci_info(index, &info);
    if (status != B_OK) return NULL;
    
    if (bar < 6) {
        // Map BAR to kernel address space
        phys_addr_t phys = info.base_registers[bar];
        size_t size = info.base_register_sizes[bar];
        
        if (phys == 0 || size == 0) {
            fprintf(stderr, "[Haiku] BAR %d not available\n", bar);
            return NULL;
        }
        
        void *virt = NULL;
        
        // Use map_physical_memory for Haiku
        #ifdef MAP_PHYSICAL_MEMORY
        status = map_physical_memory("GPU MMIO", phys, size,
                                     B_ANY_KERNEL_BLOCK_ADDRESS,
                                     B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
                                     &virt);
        if (status != B_OK) {
            fprintf(stderr, "[Haiku] Failed to map BAR %d: 0x%lx\n", bar, status);
            return NULL;
        }
        #endif
        
        fprintf(stderr, "[Haiku] Mapped BAR %d: phys=0x%lx size=0x%lx -> virt=%p\n",
                bar, phys, size, virt);
        return virt;
    }
    
    return NULL;
#else
    return NULL;
#endif
}

int os_prim_pci_unmap_resource(void *virt_addr) {
    if (!virt_addr) return 0;
    
#ifdef __HAIKU__
    // Haiku requires calling delete_area for mapped memory
    area_id area = area_for(virt_addr);
    if (area >= 0) {
        delete_area(area);
        return 0;
    }
#else
    munmap(virt_addr, 0);  // Size doesn't matter for munmap
#endif
    
    return 0;
}

/* ============================================================================
 * Interrupts (Haiku)
 * ============================================================================ */

int os_prim_register_interrupt(int irq, os_prim_interrupt_handler handler, void *data) {
    // Stub for now - interrupt support requires kernel cooperation
    fprintf(stderr, "[Haiku] Interrupt registration not yet implemented\n");
    return -1;
}

void os_prim_unregister_interrupt(int irq) {
    // Stub for now
}

/* ============================================================================
 * Display (Haiku Accelerant)
 * ============================================================================ */

int os_prim_display_init(void) {
    // Haiku accelerant system handles this
    fprintf(stderr, "[Haiku] Display initialized (via accelerant)\n");
    return 0;
}

void os_prim_display_put_pixel(int x, int y, uint32_t color) {
    // Actual pixel drawing handled by accelerant
}

/* ============================================================================
 * Synchronization (Haiku)
 * ============================================================================ */

int os_prim_lock_init(os_prim_lock *lock) {
#ifdef __HAIKU__
    if (!lock) return -1;
    *lock = create_sem(1, "gpu_lock");
    return (*lock >= 0) ? 0 : -1;
#else
    return 0;
#endif
}

int os_prim_lock(os_prim_lock lock) {
#ifdef __HAIKU__
    return (acquire_sem(lock) == B_OK) ? 0 : -1;
#else
    return 0;
#endif
}

int os_prim_unlock(os_prim_lock lock) {
#ifdef __HAIKU__
    return (release_sem(lock) == B_OK) ? 0 : -1;
#else
    return 0;
#endif
}

void os_prim_lock_destroy(os_prim_lock lock) {
#ifdef __HAIKU__
    if (lock >= 0) {
        delete_sem(lock);
    }
#endif
}

/* ============================================================================
 * Threading (Haiku)
 * ============================================================================ */

os_prim_thread os_prim_spawn_thread(const char *name,
                                     void *(*func)(void *),
                                     void *arg) {
#ifdef __HAIKU__
    thread_id thread = spawn_thread(func, name, B_NORMAL_PRIORITY, arg);
    if (thread >= 0) {
        resume_thread(thread);
        fprintf(stderr, "[Haiku] Spawned thread '%s' (ID: %ld)\n", name, thread);
    }
    return (os_prim_thread)thread;
#else
    return 0;
#endif
}

int os_prim_join_thread(os_prim_thread thread) {
#ifdef __HAIKU__
    status_t exit_code;
    status_t status = wait_for_thread((thread_id)thread, &exit_code);
    return (status == B_OK) ? (int)exit_code : -1;
#else
    return 0;
#endif
}

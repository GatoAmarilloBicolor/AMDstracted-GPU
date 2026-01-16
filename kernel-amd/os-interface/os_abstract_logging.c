/*
 * OS-Agnostic Logging Layer - Implementation
 * 
 * Multi-target logging with ring buffer, file, syslog, and callback support
 * 
 * Developed by: Haiku Imposible Team (HIT)
 */

#include "os_abstract_logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#ifdef __linux__
#include <syslog.h>
#endif

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

static struct {
    os_log_target_t target;
    os_log_level_t min_level;
    FILE *file_handle;
    char *ringbuffer;
    size_t ringbuffer_size;
    size_t ringbuffer_pos;
    void (*callback)(os_log_level_t, const char *, void *);
    void *callback_data;
    pthread_mutex_t lock;
    int initialized;
} g_logger = {
    .target = OS_LOG_TARGET_STDERR,
    .min_level = OS_LOG_INFO,
    .file_handle = NULL,
    .ringbuffer = NULL,
    .ringbuffer_size = 0,
    .ringbuffer_pos = 0,
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .initialized = 0,
};

/* ============================================================================
 * LOG LEVEL NAMES
 * ============================================================================ */

static const char* g_level_names[] = {
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL",
};

static const char* g_level_colors[] = {
    "\x1b[37m",   // TRACE - white
    "\x1b[36m",   // DEBUG - cyan
    "\x1b[32m",   // INFO - green
    "\x1b[33m",   // WARN - yellow
    "\x1b[31m",   // ERROR - red
    "\x1b[35m",   // FATAL - magenta
};

static const char* g_color_reset = "\x1b[0m";

/* ============================================================================
 * HELPER: Format timestamp
 * ============================================================================ */

static void get_timestamp(char *buf, size_t size) {
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm);
}

/* ============================================================================
 * CORE LOGGING FUNCTION
 * ============================================================================ */

void os_vlog(os_log_level_t level, const char *component,
            const char *fmt, va_list args) {
    if (level < g_logger.min_level) {
        return;  // Filtered by level
    }
    
    pthread_mutex_lock(&g_logger.lock);
    
    // Format message
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));
    
    char component_str[64] = "AMD-GPU";
    if (component) {
        snprintf(component_str, sizeof(component_str), "%s", component);
    }
    
    char level_str[16];
    snprintf(level_str, sizeof(level_str), "%s", g_level_names[level]);
    
    // Format full message
    char message[2048];
    int msg_len = snprintf(message, sizeof(message),
                          "%s [%s] [%s] ", timestamp, level_str, component_str);
    
    vsnprintf(message + msg_len, sizeof(message) - msg_len, fmt, args);
    
    size_t total_len = strlen(message);
    
    // Output to target
    switch (g_logger.target) {
    case OS_LOG_TARGET_STDERR:
        fprintf(stderr, "%s%s%s\n", g_level_colors[level], message, g_color_reset);
        fflush(stderr);
        break;
        
    case OS_LOG_TARGET_FILE:
        if (g_logger.file_handle) {
            fprintf(g_logger.file_handle, "%s\n", message);
            fflush(g_logger.file_handle);
        }
        break;
        
    case OS_LOG_TARGET_RINGBUFFER:
        if (g_logger.ringbuffer && g_logger.ringbuffer_size > 0) {
            size_t available = g_logger.ringbuffer_size - g_logger.ringbuffer_pos;
            
            if (total_len + 1 > available) {
                // Wrap around
                g_logger.ringbuffer_pos = 0;
            }
            
            if (total_len + 1 <= g_logger.ringbuffer_size) {
                memcpy(&g_logger.ringbuffer[g_logger.ringbuffer_pos], 
                      message, total_len);
                g_logger.ringbuffer_pos += total_len;
                g_logger.ringbuffer[g_logger.ringbuffer_pos++] = '\n';
            }
        }
        break;
        
    case OS_LOG_TARGET_CALLBACK:
        if (g_logger.callback) {
            g_logger.callback(level, message, g_logger.callback_data);
        }
        break;
        
    case OS_LOG_TARGET_SYSLOG:
#ifdef __linux__
        syslog(LOG_INFO, "%s", message);
#else
        fprintf(stderr, "%s\n", message);
#endif
        break;
        
    case OS_LOG_TARGET_KERNEL_LOG:
#ifdef __linux__
        // Write to kernel ring buffer
        int fd = open("/dev/kmsg", O_WRONLY);
        if (fd >= 0) {
            dprintf(fd, "<6>%s\n", message);
            close(fd);
        }
#endif
        fprintf(stderr, "%s\n", message);
        break;
        
    case OS_LOG_TARGET_HAIKU_LOG:
#ifdef __HAIKU__
        dprintf("%s\n", message);
#endif
        fprintf(stderr, "%s\n", message);
        break;
        
    default:
        fprintf(stderr, "%s\n", message);
        break;
    }
    
    pthread_mutex_unlock(&g_logger.lock);
}

void os_log(os_log_level_t level, const char *component,
           const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    os_vlog(level, component, fmt, args);
    va_end(args);
}

/* ============================================================================
 * LOGGER CONFIGURATION
 * ============================================================================ */

int os_logger_init(const os_logger_config_t *config) {
    if (!config) {
        return -1;
    }
    
    pthread_mutex_lock(&g_logger.lock);
    
    g_logger.target = config->target;
    g_logger.min_level = config->min_level;
    
    // Setup target
    switch (config->target) {
    case OS_LOG_TARGET_FILE:
        if (config->file_path) {
            g_logger.file_handle = fopen(config->file_path, "a");
            if (!g_logger.file_handle) {
                pthread_mutex_unlock(&g_logger.lock);
                return -1;
            }
        }
        break;
        
    case OS_LOG_TARGET_RINGBUFFER:
        if (config->max_buffer_size > 0) {
            g_logger.ringbuffer = malloc(config->max_buffer_size);
            if (!g_logger.ringbuffer) {
                pthread_mutex_unlock(&g_logger.lock);
                return -1;
            }
            g_logger.ringbuffer_size = config->max_buffer_size;
            g_logger.ringbuffer_pos = 0;
            memset(g_logger.ringbuffer, 0, config->max_buffer_size);
        }
        break;
        
    case OS_LOG_TARGET_CALLBACK:
        g_logger.callback = config->callback;
        g_logger.callback_data = config->callback_data;
        break;
        
    case OS_LOG_TARGET_SYSLOG:
#ifdef __linux__
        openlog("amdgpu", LOG_PID | LOG_CONS, LOG_DAEMON);
#endif
        break;
        
    default:
        break;
    }
    
    g_logger.initialized = 1;
    pthread_mutex_unlock(&g_logger.lock);
    
    os_log(OS_LOG_INFO, "LOGGER", "Logging initialized (target=%d, level=%d)\n",
          config->target, config->min_level);
    
    return 0;
}

void os_logger_set_level(os_log_level_t level) {
    pthread_mutex_lock(&g_logger.lock);
    g_logger.min_level = level;
    pthread_mutex_unlock(&g_logger.lock);
}

os_log_level_t os_logger_get_level(void) {
    return g_logger.min_level;
}

int os_logger_enable_component(const char *component, int enabled) {
    // TODO: Implement per-component filtering
    return 0;
}

void os_logger_flush(void) {
    pthread_mutex_lock(&g_logger.lock);
    
    if (g_logger.file_handle) {
        fflush(g_logger.file_handle);
    }
    
    fflush(stderr);
    
    pthread_mutex_unlock(&g_logger.lock);
}

const char* os_logger_get_buffer(size_t *size) {
    pthread_mutex_lock(&g_logger.lock);
    
    if (!g_logger.ringbuffer) {
        pthread_mutex_unlock(&g_logger.lock);
        return NULL;
    }
    
    *size = g_logger.ringbuffer_pos;
    const char *result = g_logger.ringbuffer;
    
    pthread_mutex_unlock(&g_logger.lock);
    
    return result;
}

void os_logger_clear_buffer(void) {
    pthread_mutex_lock(&g_logger.lock);
    
    if (g_logger.ringbuffer) {
        memset(g_logger.ringbuffer, 0, g_logger.ringbuffer_size);
        g_logger.ringbuffer_pos = 0;
    }
    
    pthread_mutex_unlock(&g_logger.lock);
}

void os_logger_fini(void) {
    pthread_mutex_lock(&g_logger.lock);
    
    if (g_logger.file_handle) {
        fclose(g_logger.file_handle);
        g_logger.file_handle = NULL;
    }
    
    if (g_logger.ringbuffer) {
        free(g_logger.ringbuffer);
        g_logger.ringbuffer = NULL;
    }
    
#ifdef __linux__
    closelog();
#endif
    
    g_logger.initialized = 0;
    pthread_mutex_unlock(&g_logger.lock);
}

/* ============================================================================
 * HEX DUMP
 * ============================================================================ */

void os_log_hexdump(os_log_level_t level, const char *component,
                   const void *data, size_t size, const char *prefix) {
    if (level < g_logger.min_level) {
        return;
    }
    
    const uint8_t *bytes = (const uint8_t *)data;
    char line[256];
    
    os_log(level, component, "%s (size=%zu):", prefix ? prefix : "Data", size);
    
    for (size_t i = 0; i < size; i += 16) {
        int len = 0;
        len += snprintf(line + len, sizeof(line) - len, "%08zx: ", i);
        
        // Hex portion
        for (int j = 0; j < 16 && i + j < size; j++) {
            len += snprintf(line + len, sizeof(line) - len, "%02x ", bytes[i + j]);
        }
        
        // ASCII portion
        len += snprintf(line + len, sizeof(line) - len, "  ");
        for (int j = 0; j < 16 && i + j < size; j++) {
            char c = bytes[i + j];
            if (c >= 32 && c < 127) {
                line[len++] = c;
            } else {
                line[len++] = '.';
            }
        }
        
        line[len] = '\0';
        os_log(level, component, "%s", line);
    }
}

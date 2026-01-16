/*
 * OS-Agnostic Logging Layer
 * 
 * Provides structured logging with levels, filtering, and formatting
 * Adapts to OS capabilities: kernel logs (Linux), dprintf (Haiku), etc.
 * 
 * Developed by: Haiku Imposible Team (HIT)
 */

#ifndef OS_ABSTRACT_LOGGING_H
#define OS_ABSTRACT_LOGGING_H

#include <stdarg.h>

/* ============================================================================
 * LOG LEVELS
 * ============================================================================ */

typedef enum {
    OS_LOG_TRACE = 0,
    OS_LOG_DEBUG = 1,
    OS_LOG_INFO = 2,
    OS_LOG_WARN = 3,
    OS_LOG_ERROR = 4,
    OS_LOG_FATAL = 5,
} os_log_level_t;

/* ============================================================================
 * LOGGER CONFIGURATION
 * ============================================================================ */

typedef enum {
    OS_LOG_TARGET_STDERR,      // stderr (default)
    OS_LOG_TARGET_SYSLOG,      // System log (Linux, BSD)
    OS_LOG_TARGET_KERNEL_LOG,  // Kernel ring buffer (Linux)
    OS_LOG_TARGET_HAIKU_LOG,   // Haiku dprintf
    OS_LOG_TARGET_FILE,        // File output
    OS_LOG_TARGET_RINGBUFFER,  // In-memory ring buffer
    OS_LOG_TARGET_CALLBACK,    // Custom callback
} os_log_target_t;

typedef struct {
    os_log_target_t target;
    os_log_level_t min_level;
    uint32_t max_buffer_size;
    const char *file_path;     // For FILE target
    void (*callback)(os_log_level_t level, const char *msg, void *user_data);
    void *callback_data;
} os_logger_config_t;

/* ============================================================================
 * LOGGER API
 * ============================================================================ */

/**
 * Initialize logger with configuration
 */
int os_logger_init(const os_logger_config_t *config);

/**
 * Log message with printf-style formatting
 */
void os_log(os_log_level_t level, const char *component,
           const char *fmt, ...);

/**
 * Log with va_list
 */
void os_vlog(os_log_level_t level, const char *component,
            const char *fmt, va_list args);

/**
 * Convenience macros
 */
#define OS_LOG_TRACE(comp, fmt, ...) os_log(OS_LOG_TRACE, comp, fmt, ##__VA_ARGS__)
#define OS_LOG_DEBUG(comp, fmt, ...) os_log(OS_LOG_DEBUG, comp, fmt, ##__VA_ARGS__)
#define OS_LOG_INFO(comp, fmt, ...)  os_log(OS_LOG_INFO, comp, fmt, ##__VA_ARGS__)
#define OS_LOG_WARN(comp, fmt, ...)  os_log(OS_LOG_WARN, comp, fmt, ##__VA_ARGS__)
#define OS_LOG_ERROR(comp, fmt, ...) os_log(OS_LOG_ERROR, comp, fmt, ##__VA_ARGS__)
#define OS_LOG_FATAL(comp, fmt, ...) os_log(OS_LOG_FATAL, comp, fmt, ##__VA_ARGS__)

/**
 * Set minimum log level (for filtering)
 */
void os_logger_set_level(os_log_level_t level);

/**
 * Get current log level
 */
os_log_level_t os_logger_get_level(void);

/**
 * Enable/disable logging for specific component
 */
int os_logger_enable_component(const char *component, int enabled);

/**
 * Flush any pending log output
 */
void os_logger_flush(void);

/**
 * Get ring buffer contents (for OS_LOG_TARGET_RINGBUFFER)
 */
const char* os_logger_get_buffer(size_t *size);

/**
 * Clear ring buffer
 */
void os_logger_clear_buffer(void);

/**
 * Shutdown logger
 */
void os_logger_fini(void);

/* ============================================================================
 * ASSERT MACRO (uses logging)
 * ============================================================================ */

#define OS_ASSERT(cond, comp, msg) \
    do { \
        if (!(cond)) { \
            os_log(OS_LOG_FATAL, comp, "ASSERTION FAILED: %s at %s:%d - %s\n", \
                  #cond, __FILE__, __LINE__, msg); \
            abort(); \
        } \
    } while (0)

/* ============================================================================
 * HEX DUMP HELPER
 * ============================================================================ */

void os_log_hexdump(os_log_level_t level, const char *component,
                   const void *data, size_t size, const char *prefix);

#endif // OS_ABSTRACT_LOGGING_H

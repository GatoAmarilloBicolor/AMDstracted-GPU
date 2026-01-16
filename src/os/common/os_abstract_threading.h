/*
 * OS-Agnostic Threading Layer
 * 
 * Abstracts thread creation, synchronization across platforms
 * Supports: POSIX pthreads, Haiku threads, Windows threads
 * 
 * Developed by: Haiku Imposible Team (HIT)
 */

#ifndef OS_ABSTRACT_THREADING_H
#define OS_ABSTRACT_THREADING_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================================
 * THREAD TYPES & ENUMS
 * ============================================================================ */

typedef int32_t os_thread_id_t;
typedef int os_thread_priority_t;

typedef enum {
    OS_THREAD_PRIORITY_IDLE = 0,
    OS_THREAD_PRIORITY_LOW = 1,
    OS_THREAD_PRIORITY_NORMAL = 2,
    OS_THREAD_PRIORITY_HIGH = 3,
    OS_THREAD_PRIORITY_URGENT = 4,  // For interrupt handlers
    OS_THREAD_PRIORITY_REALTIME = 5,
} os_thread_priority_level_t;

typedef int (*os_thread_func_t)(void *arg);

/* ============================================================================
 * THREAD MANAGEMENT
 * ============================================================================ */

/**
 * Create and start a new thread
 * 
 * @param name          Thread name (for debugging)
 * @param priority      Priority level
 * @param func          Thread function
 * @param arg           Argument to pass to thread
 * @return Thread ID, or < 0 on error
 */
os_thread_id_t os_thread_create(const char *name, os_thread_priority_level_t priority,
                                os_thread_func_t func, void *arg);

/**
 * Wait for thread to complete
 * 
 * @param tid           Thread ID
 * @param exit_code     Pointer to store thread exit code
 * @return 0 on success, < 0 on error
 */
int os_thread_join(os_thread_id_t tid, int *exit_code);

/**
 * Cancel a thread
 */
int os_thread_cancel(os_thread_id_t tid);

/**
 * Get current thread ID
 */
os_thread_id_t os_thread_self(void);

/**
 * Sleep current thread (microseconds)
 */
void os_thread_usleep(uint32_t us);

/* ============================================================================
 * SYNCHRONIZATION PRIMITIVES
 * ============================================================================ */

typedef int32_t os_semaphore_t;
typedef int32_t os_mutex_t;
typedef int32_t os_condition_variable_t;

/**
 * Create a semaphore
 */
os_semaphore_t os_semaphore_create(uint32_t initial_count, const char *name);

/**
 * Acquire semaphore (wait if count == 0)
 */
int os_semaphore_acquire(os_semaphore_t sem, uint32_t timeout_ms);

/**
 * Release semaphore (increment count)
 */
int os_semaphore_release(os_semaphore_t sem);

/**
 * Destroy semaphore
 */
void os_semaphore_destroy(os_semaphore_t sem);

/**
 * Create a mutex
 */
os_mutex_t os_mutex_create(const char *name);

/**
 * Lock mutex
 */
int os_mutex_lock(os_mutex_t mutex, uint32_t timeout_ms);

/**
 * Unlock mutex
 */
int os_mutex_unlock(os_mutex_t mutex);

/**
 * Destroy mutex
 */
void os_mutex_destroy(os_mutex_t mutex);

/**
 * Create a condition variable
 */
os_condition_variable_t os_cond_create(const char *name);

/**
 * Wait on condition (must hold mutex)
 */
int os_cond_wait(os_condition_variable_t cond, os_mutex_t mutex,
                uint32_t timeout_ms);

/**
 * Signal one waiting thread
 */
int os_cond_signal(os_condition_variable_t cond);

/**
 * Broadcast to all waiting threads
 */
int os_cond_broadcast(os_condition_variable_t cond);

/**
 * Destroy condition variable
 */
void os_cond_destroy(os_condition_variable_t cond);

/* ============================================================================
 * THREAD-LOCAL STORAGE
 * ============================================================================ */

typedef int32_t os_tls_key_t;

/**
 * Allocate thread-local storage key
 */
os_tls_key_t os_tls_alloc(void (*destructor)(void *));

/**
 * Set thread-local value
 */
int os_tls_set(os_tls_key_t key, void *value);

/**
 * Get thread-local value
 */
void* os_tls_get(os_tls_key_t key);

/**
 * Free thread-local storage key
 */
void os_tls_free(os_tls_key_t key);

/* ============================================================================
 * THREAD POOL (for worker threads)
 * ============================================================================ */

typedef struct {
    uint32_t num_workers;
    uint32_t queue_size;
    os_thread_priority_level_t priority;
} os_threadpool_config_t;

typedef void* os_threadpool_t;
typedef void (*os_task_func_t)(void *arg);

/**
 * Create a thread pool
 */
os_threadpool_t os_threadpool_create(const os_threadpool_config_t *config);

/**
 * Submit task to thread pool
 */
int os_threadpool_submit(os_threadpool_t pool, os_task_func_t func, void *arg);

/**
 * Wait for all tasks to complete
 */
int os_threadpool_join(os_threadpool_t pool, uint32_t timeout_ms);

/**
 * Destroy thread pool
 */
void os_threadpool_destroy(os_threadpool_t pool);

#endif // OS_ABSTRACT_THREADING_H

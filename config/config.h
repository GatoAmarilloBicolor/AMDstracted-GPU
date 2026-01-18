#ifndef CONFIG_H
#define CONFIG_H

// Build configuration for AMDGPU_Abstracted

// OS detection
#if defined(__HAIKU__)
#define OS_HAIKU 1
#define OS_LINUX 0
#define OS_FREEBSD 0
#elif defined(__linux__)
#define OS_HAIKU 0
#define OS_LINUX 1
#define OS_FREEBSD 0
#elif defined(__FreeBSD__)
#define OS_HAIKU 0
#define OS_LINUX 0
#define OS_FREEBSD 1
#else
#error "Unsupported OS"
#endif

// Userland mode
#ifndef USERLAND_MODE
#define USERLAND_MODE 0
#endif

// Debug settings
#define AMD_DEBUG 0
#define AMD_LOG_LEVEL 1

// Haiku-specific includes
#if defined(__HAIKU__)
#include <GraphicsDefs.h>
#endif

#endif // CONFIG_H

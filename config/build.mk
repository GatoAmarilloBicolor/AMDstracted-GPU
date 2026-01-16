# ============================================================================
# Build Configuration - AMDGPU_Abstracted
# Central configuration for all build targets
# ============================================================================

# Detect OS
DETECTED_OS := $(shell uname -s | tr '[:upper:]' '[:lower:]')
OS ?= $(DETECTED_OS)

# Compiler
CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -fPIC -DUSERLAND_MODE=0

# Include paths (relative to project root)
INCLUDE_PATHS := -I. \
                 -I./src \
                 -I./src/amd \
                 -I./src/common \
                 -I./src/os \
                 -I./kernel-amd/os-interface \
                 -I./kernel-amd/os-primitives

# OS-specific flags
ifeq ($(OS),linux)
    CFLAGS += -D__LINUX__
    LDLIBS := -lm -pthread -lnetwork
endif
ifeq ($(OS),haiku)
    CFLAGS += -D__HAIKU__
    LDLIBS := -lroot -lnetwork -lbe
endif
ifeq ($(OS),freebsd)
    CFLAGS += -D__FreeBSD__
    LDLIBS := -lm -pthread
endif

# ============================================================================
# Component Source Files
# ============================================================================

# Shader Compiler
SHADER_SRCS := src/amd/shader_compiler.c
SHADER_OBJS := $(SHADER_SRCS:.c=.o)

# RADV Backend
RADV_SRCS := src/amd/radv_backend.c
RADV_OBJS := $(RADV_SRCS:.c=.o)

# Zink Layer
ZINK_SRCS := src/amd/zink_layer.c
ZINK_OBJS := $(ZINK_SRCS:.c=.o)

# HAL
HAL_SRCS := src/amd/hal.c src/amd/objgpu.c src/amd/rmapi.c src/amd/rmapi_server.c
HAL_OBJS := $(HAL_SRCS:.c=.o)

# IP Blocks
IPBLOCK_SRCS := src/amd/gmc_v10.c src/amd/gfx_v10.c src/amd/vcn_v2.c
IPBLOCK_OBJS := $(IPBLOCK_SRCS:.c=.o)

# Common
COMMON_SRCS := src/common/ipc_lib.c src/common/resserv.c
COMMON_OBJS := $(COMMON_SRCS:.c=.o)

# OS Primitives (selected based on detected OS)
ifeq ($(OS),linux)
    OS_SRCS := kernel-amd/os-interface/linux/os_interface_linux.c \
               kernel-amd/os-primitives/linux/os_primitives_linux.c
else ifeq ($(OS),haiku)
    OS_SRCS := kernel-amd/os-primitives/haiku/os_primitives_haiku.c
else ifeq ($(OS),freebsd)
    OS_SRCS := kernel-amd/os-primitives/freebsd/os_primitives_freebsd.c
else
    OS_SRCS := kernel-amd/os-interface/linux/os_interface_linux.c \
               kernel-amd/os-primitives/linux/os_primitives_linux.c
endif
OS_OBJS := $(OS_SRCS:.c=.o)

# DRM Shim
DRM_SRCS := drm-shim/drm_shim.c
DRM_OBJS := $(DRM_SRCS:.c=.o)

# Consolidate
ALL_SRCS := $(SHADER_SRCS) $(RADV_SRCS) $(ZINK_SRCS) $(HAL_SRCS) \
            $(IPBLOCK_SRCS) $(COMMON_SRCS) $(OS_SRCS) $(DRM_SRCS)
ALL_OBJS := $(ALL_SRCS:.c=.o)

# ============================================================================
# Output Targets
# ============================================================================

LIBAMDGPU_SO := libamdgpu.so
RMAPI_SERVER := rmapi_server
RMAPI_CLIENT := rmapi_client_demo

# ============================================================================
# Compilation Rules
# ============================================================================

%.o: %.c
	@mkdir -p $(dir $@)
	@echo "[CC] $<"
	$(CC) $(CFLAGS) $(INCLUDE_PATHS) -c $< -o $@

# ============================================================================
# Phony targets
# ============================================================================

.PHONY: help info clean all

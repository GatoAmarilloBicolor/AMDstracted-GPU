# Top-level Makefile for AMDGPU_Abstracted (HIT Edition)
# Fixed version with correct paths

# 1. Detect OS automatically (POSIX compliant)
DETECTED_OS := $(shell uname -s | tr '[:upper:]' '[:lower:]')

# 2. Map OSes to supported "Brains" (OS Adapters)
ifeq ($(DETECTED_OS),linux)
  MAPPED_OS := linux
else ifeq ($(DETECTED_OS),freebsd)
  MAPPED_OS := freebsd
else ifeq ($(DETECTED_OS),haiku)
  MAPPED_OS := haiku
else ifeq ($(DETECTED_OS),redox)
  MAPPED_OS := linux
else ifeq ($(DETECTED_OS),fuchsia)
  MAPPED_OS := fuchsia
else ifeq ($(DETECTED_OS),darwin)
  MAPPED_OS := haiku
else
  MAPPED_OS := generic_posix
endif

OS ?= $(MAPPED_OS)

# 3. Handle Library Tweaks (Haiku, etc.)
ifeq ($(OS),haiku)
   LDFLAGS += -lroot -lnetwork
   PTHREAD_LIBS = # Already in libroot
   # Special for Haiku Kernel/Accelerant
   HAIKU_LDFLAGS = -lbe -ltranslation
    CC = clang
    CXX = clang++
    HAIKU_INCLUDES = -I/boot/system/develop/headers -I/boot/system/develop/headers/posix -I/boot/system/develop/headers/os -I/boot/system/develop/headers/os/interface
endif

$(info [HIT] Building for OS: $(OS) (Detected: $(DETECTED_OS)))

# 4. Set Directory Paths
ifeq ($(wildcard src/os/$(OS)),)
  OS_DIR_SUFFIX = linux
  OS_INTERFACE_DIR = src/os/linux
  OS_PRIMITIVES_DIR = src/os/linux
else
  OS_DIR_SUFFIX = $(OS)
  OS_INTERFACE_DIR = src/os/$(OS)
  OS_PRIMITIVES_DIR = src/os/$(OS)
endif

# 5. Build Options
USERLAND_MODE ?= 0

# Define OS flag for conditional compilation
ifeq ($(OS),linux)
  CFLAGS += -D__LINUX__
  CXXFLAGS += -D__LINUX__
else ifeq ($(OS),haiku)
  CFLAGS += -D__HAIKU__
  CXXFLAGS += -D__HAIKU__
else ifeq ($(OS),freebsd)
  CFLAGS += -D__FreeBSD__
  CXXFLAGS += -D__FreeBSD__
endif

CFLAGS += -DUSERLAND_MODE=$(USERLAND_MODE) -std=c99 -include config/config.h -I. \
          -Iconfig -I$(OS_INTERFACE_DIR) -I$(OS_PRIMITIVES_DIR) \
          -Icore -Icore/gpu -Icore/hal -Icore/rmapi -Icore/ipc -Ios \
          $(HAIKU_INCLUDES)
CXXFLAGS += -DUSERLAND_MODE=$(USERLAND_MODE) -std=c++11 -include config/config.h -I. \
            -Iconfig -I$(OS_INTERFACE_DIR) -I$(OS_PRIMITIVES_DIR) \
            -Isrc -Isrc/common -Isrc/amd -Isrc/amd/hal -Isrc/amd/rmapi \
            $(HAIKU_INCLUDES)

# Directories
CORE_DIR = core
DRIVERS_DIR = drivers/amdgpu

# List of Objects to build
SRC_OBJS = $(CORE_DIR)/gpu/objgpu.o \
           $(CORE_DIR)/hal/hal/hal.o \
           $(CORE_DIR)/rmapi/rmapi/rmapi.o \
           $(CORE_DIR)/rmapi/rmapi/rmapi_server.o \
           $(CORE_DIR)/ipc/ipc_lib.o \
           $(DRIVERS_DIR)/amdgpu_gem_userland.o \
           $(DRIVERS_DIR)/amdgpu_kms_userland.o \
           $(DRIVERS_DIR)/ip_blocks/gmc_v10.o \
           $(DRIVERS_DIR)/ip_blocks/gfx_v10.o \
           $(DRIVERS_DIR)/shader_compiler/shader_compiler.o \
           $(DRIVERS_DIR)/radv_backend/radv_backend.o \
           $(DRIVERS_DIR)/zink_layer/zink_layer.o

OS_OBJS = os/$(OS_DIR_SUFFIX)/os_interface_$(OS_DIR_SUFFIX).o \
           os/$(OS_DIR_SUFFIX)/os_primitives_$(OS_DIR_SUFFIX).o

# 6. Build Targets
TARGETS = libamdgpu.so rmapi_server rmapi_client_demo
ifdef __HAIKU__
  TARGETS += amdgpu_hit
endif

all: $(TARGETS)

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c $< -o $@

libamdgpu.so: $(SRC_OBJS) $(OS_OBJS)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

rmapi_server: $(SRC_DIR)/rmapi/rmapi_server.o \
              $(SRC_DIR)/hal/hal.o \
              $(COMMON_DIR)/gpu/objgpu.o \
              $(SRC_DIR)/rmapi/rmapi.o \
              $(COMMON_DIR)/resource/resserv.o \
              $(SRC_DIR)/ip_blocks/gmc_v10.o \
              $(SRC_DIR)/ip_blocks/gfx_v10.o \
              $(COMMON_DIR)/ipc/ipc_lib.o \
              $(OS_OBJS)
	$(CC) $(CFLAGS) -Wall $^ $(PTHREAD_LIBS) $(LDFLAGS) -o $@

rmapi_client_demo: examples/rmapi_client_demo.c \
                   $(COMMON_DIR)/gpu/objgpu.o \
                   $(SRC_DIR)/hal/hal.o \
                   $(SRC_DIR)/amdgpu_gem_userland.o \
                   $(SRC_DIR)/amdgpu_kms_userland.o \
                   $(COMMON_DIR)/resource/resserv.o \
                   $(SRC_DIR)/rmapi/rmapi.o \
                   $(SRC_DIR)/ip_blocks/gmc_v10.o \
                   $(SRC_DIR)/ip_blocks/gfx_v10.o \
                   $(SRC_DIR)/shader_compiler/shader_compiler.o \
                   $(SRC_DIR)/radv_backend/radv_backend.o \
                   $(SRC_DIR)/zink_layer/zink_layer.o \
                   $(COMMON_DIR)/ipc/ipc_lib.o \
                   $(OS_OBJS)
	$(CC) $(CFLAGS) -Wall $^ $(PTHREAD_LIBS) $(LDFLAGS) -o $@

# --- Haiku Specific Specialist Binaries ---
amdgpu_hit: os/haiku/addon/AmdAddon.o $(OS_OBJS)
	$(CXX) -shared -o $@ $^ $(LDFLAGS) $(HAIKU_LDFLAGS)

# amdgpu_hit.accelerant: src/os/haiku/accelerant/AmdAccelerant.o $(COMMON_DIR)/ipc/ipc_lib.o $(OS_OBJS)
# 	$(CXX) -shared -o $@ $^ $(LDFLAGS) $(HAIKU_LDFLAGS)

# --- DRM Compatibility Shim for RADV/Zink ---
drm-shim/libdrm_amdgpu.so: drm-shim/drm_shim.c $(COMMON_DIR)/ipc/ipc_lib.o $(OS_OBJS)
	$(CC) -shared -fPIC -o $@ $^ $(LDFLAGS) -I./src/common

drm-shim: drm-shim/libdrm_amdgpu.so

clean:
	rm -f *.o *.so *.ko 
	find . -name "*.o" -type f -delete
	rm -f rmapi_server rmapi_client_demo amdgpu_hit amdgpu_hit.accelerant

.PHONY: all clean drm-shim

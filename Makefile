# Top-level Makefile for AMDGPU_Abstracted

# 1. Detect OS automatically (POSIX compliant)
DETECTED_OS := $(shell uname -s | tr '[:upper:]' '[:lower:]')

# 2. Map OSes to supported "Brains" (OS Adapters)
ifeq ($(DETECTED_OS),linux)
  MAPPED_OS := linux
else ifeq ($(DETECTED_OS),freebsd)
  MAPPED_OS := linux
else ifeq ($(DETECTED_OS),haiku)
  MAPPED_OS := haiku
else ifeq ($(DETECTED_OS),redox)
  MAPPED_OS := linux
else ifeq ($(DETECTED_OS),fuchsia)
  MAPPED_OS := fuchsia
else ifeq ($(DETECTED_OS),darwin)
  MAPPED_OS := haiku
else
  # "Exotic" POSIX system fallback
  MAPPED_OS := generic_posix
endif

# Allow user to override OS via 'make OS=...'
OS ?= $(MAPPED_OS)

# 3. Handle Library Tweaks (Haiku, etc.)
ifeq ($(OS),haiku)
  LDFLAGS += -lroot -lnetwork
  PTHREAD_LIBS = # Already in libroot
else
  PTHREAD_LIBS = -lpthread
  # If we're on an exotic system, we just use the Linux logic but strip it down
  ifeq ($(OS),generic_posix)
    LDFLAGS += 
  endif
endif

$(info [HIT] Building for OS: $(OS) (Detected: $(DETECTED_OS)))

# 4. Set Directory Paths
# If we don't have a specific adapter for the OS, we use the Linux one 
# as a "Generic POSIX" template.
ifeq ($(wildcard kernel-amd/os-interface/$(OS)),)
  $(warning [HIT] OS adapter for $(OS) not found. Using Generic POSIX template.)
  OS_DIR_SUFFIX = linux
  OS_INTERFACE_DIR = kernel-amd/os-interface/linux
  OS_PRIMITIVES_DIR = kernel-amd/os-primitives/linux
else
  OS_DIR_SUFFIX = $(OS)
  OS_INTERFACE_DIR = kernel-amd/os-interface/$(OS)
  OS_PRIMITIVES_DIR = kernel-amd/os-primitives/$(OS)
endif

# 5. Build Options
USERLAND_MODE ?= 0
CFLAGS += -DUSERLAND_MODE=$(USERLAND_MODE) -std=c99 -include config.h

SRC_DIR = src/amd
COMMON_DIR = src/common

# List of Objects to build
SRC_OBJS = $(SRC_DIR)/objgpu.o $(SRC_DIR)/hal.o $(SRC_DIR)/amdgpu_gem_userland.o \
           $(SRC_DIR)/amdgpu_kms_userland.o $(SRC_DIR)/resserv.o $(SRC_DIR)/rmapi.o \
           $(SRC_DIR)/rmapi_server.o $(COMMON_DIR)/ipc_lib.o

OS_OBJS = $(OS_INTERFACE_DIR)/os_interface_$(OS_DIR_SUFFIX).o \
          $(OS_PRIMITIVES_DIR)/os_primitives_$(OS_DIR_SUFFIX).o

# 6. Build Targets
all: libamdgpu.so rmapi_server rmapi_client_demo

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

libamdgpu.so: $(SRC_OBJS) $(OS_OBJS)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

rmapi_server: $(SRC_DIR)/rmapi_server.o $(SRC_DIR)/hal.o $(SRC_DIR)/objgpu.o \
               $(SRC_DIR)/rmapi.o $(SRC_DIR)/resserv.o $(COMMON_DIR)/ipc_lib.o $(OS_OBJS)
	$(CC) -I. -Ikernel-amd/os-interface -Ikernel-amd/os-primitives -Wall $^ $(PTHREAD_LIBS) $(LDFLAGS) -o $@

rmapi_client_demo: rmapi_client_demo.c $(filter-out $(SRC_DIR)/rmapi_server.o, $(SRC_OBJS)) $(OS_OBJS)
	$(CC) -I. -Ikernel-amd/os-interface -Ikernel-amd/os-primitives -Wall $^ $(PTHREAD_LIBS) $(LDFLAGS) -o $@

clean:
	rm -f *.o *.so *.ko $(SRC_DIR)/*.o $(COMMON_DIR)/*.o \
	kernel-amd/os-interface/*/*.o kernel-amd/os-primitives/*/*.o \
	rmapi_server rmapi_client_demo

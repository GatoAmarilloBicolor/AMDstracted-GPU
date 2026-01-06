# Top-level Makefile for AMDGPU_Abstracted

# Detect OS automatically if not specified (POSIX compliant)
DETECTED_OS := $(shell uname -s | tr '[:upper:]' '[:lower:]')

# Map OSes to supported adapters (expand as needed)
ifeq ($(DETECTED_OS),linux)
  MAPPED_OS := linux
else ifeq ($(DETECTED_OS),freebsd)
  MAPPED_OS := linux  # Uses similar APIs
else ifeq ($(DETECTED_OS),darwin)
  MAPPED_OS := haiku  # Placeholder for macOS-like
else ifeq ($(DETECTED_OS),haiku)
  MAPPED_OS := haiku
else ifeq ($(DETECTED_OS),redox)
  MAPPED_OS := linux  # Redox is POSIX-compatible, fallback to linux
else ifeq ($(DETECTED_OS),fuchsia)
  MAPPED_OS := fuchsia  # Fuchsia uses Zircon, not POSIX
else ifeq ($(DETECTED_OS),netbsd)
  MAPPED_OS := linux  # BSD family, POSIX
else ifeq ($(DETECTED_OS),openbsd)
  MAPPED_OS := linux
else ifeq ($(DETECTED_OS),dragonfly)
  MAPPED_OS := linux
else ifeq ($(DETECTED_OS),solaris)
  MAPPED_OS := linux  # illumos/Solaris
else ifeq ($(DETECTED_OS),minix)
  MAPPED_OS := linux  # Minix is POSIX
else
  MAPPED_OS := linux  # Default fallback for other POSIX systems
endif

OS ?= $(MAPPED_OS)

$(info Detected OS: $(DETECTED_OS), mapped to: $(MAPPED_OS), using OS=$(OS))

# Check if OS adapter exists; if not, warn and use stubs
ifeq ($(wildcard kernel-amd/os-interface/$(OS)),)
$(warning OS adapter for $(OS) not found, using linux as fallback)
OS := linux
endif
ifeq ($(wildcard kernel-amd/os-primitives/$(OS)),)
$(warning OS primitives for $(OS) not found, using linux as fallback)
OS := linux
endif

# POSIX userland mode for testing (no kernel deps)
USERLAND_MODE ?= 0
CFLAGS += -DUSERLAND_MODE=$(USERLAND_MODE) -std=c99 -include config.h
SRC_DIR = src/amd
COMMON_DIR = src/common
OS_INTERFACE_DIR = kernel-amd/os-interface/$(OS)
OS_PRIMITIVES_DIR = kernel-amd/os-primitives/$(OS)
KERNEL_DIR = kernel-amd

SRC_OBJS = $(SRC_DIR)/objgpu.o $(SRC_DIR)/hal.o $(SRC_DIR)/amdgpu_gem_userland.o $(SRC_DIR)/amdgpu_kms_userland.o $(SRC_DIR)/resserv.o $(SRC_DIR)/rmapi.o $(SRC_DIR)/rmapi_server.o $(COMMON_DIR)/ipc_lib.o
OS_OBJS = $(OS_INTERFACE_DIR)/os_interface_$(OS).o $(OS_PRIMITIVES_DIR)/os_primitives_$(OS).o
KERNEL_OBJS = # Add kernel interface objs

# Build target
all: libamdgpu.so

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

libamdgpu.so: $(SRC_OBJS) $(OS_OBJS) $(KERNEL_OBJS)
	$(CC) -shared -o $@ $^

clean:
	rm -f *.o *.ko src/amd/*.o kernel-amd/os-interface/*/*.o kernel-amd/os-primitives/*/*.o
real_compute: real_compute.c $(filter-out src/amd/rmapi_server.o, $(SRC_OBJS)) $(OS_OBJS)
	$(CC) -I. -Ikernel-amd/os-interface -Ikernel-amd/os-primitives -Wall $< $(filter-out src/amd/rmapi_server.o, $(SRC_OBJS)) $(OS_OBJS) -o $@

rmapi_server: $(SRC_DIR)/rmapi_server.o $(SRC_DIR)/hal.o $(SRC_DIR)/objgpu.o $(COMMON_DIR)/ipc_lib.o $(OS_OBJS)
	$(CC) -I. -Ikernel-amd/os-interface -Ikernel-amd/os-primitives -Wall $^ -lpthread -o $@

rmapi_client_demo: rmapi_client_demo.c src/common/ipc_lib.o $(SRC_OBJS) $(OS_OBJS)
	$(CC) -I. -Wall rmapi_client_demo.c src/common/ipc_lib.o $(SRC_OBJS) $(OS_OBJS) -o $@

vkinfo_amd: vkinfo_amd.c $(filter-out src/amd/rmapi_server.o, $(SRC_OBJS)) $(OS_OBJS)
	$(CC) -I. -Ikernel-amd/os-interface -Ikernel-amd/os-primitives -Wall $< $(filter-out src/amd/rmapi_server.o, $(SRC_OBJS)) $(OS_OBJS) -o $@

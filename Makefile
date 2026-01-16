# Top-level Makefile for AMDGPU_Abstracted (HIT Edition)

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
else
  PTHREAD_LIBS = -lpthread
  LDFLAGS += -lrt
endif

$(info [HIT] Building for OS: $(OS) (Detected: $(DETECTED_OS)))

# 4. Set Directory Paths
ifeq ($(wildcard kernel-amd/os-interface/$(OS)),)
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
CFLAGS += -DUSERLAND_MODE=$(USERLAND_MODE) -std=c99 -include config.h -I. -Ikernel-amd/os-interface -Ikernel-amd/os-primitives
CXXFLAGS += -DUSERLAND_MODE=$(USERLAND_MODE) -std=c++11 -include config.h -I. -Ikernel-amd/os-interface -Ikernel-amd/os-primitives

SRC_DIR = src/amd
COMMON_DIR = src/common

# List of Objects to build
SRC_OBJS = $(SRC_DIR)/objgpu.o $(SRC_DIR)/hal.o $(SRC_DIR)/amdgpu_gem_userland.o \
           $(SRC_DIR)/amdgpu_kms_userland.o $(SRC_DIR)/resserv.o $(SRC_DIR)/rmapi.o \
           $(SRC_DIR)/rmapi_server.o $(COMMON_DIR)/ipc_lib.o

OS_OBJS = $(OS_INTERFACE_DIR)/os_interface_$(OS_DIR_SUFFIX).o \
          $(OS_PRIMITIVES_DIR)/os_primitives_$(OS_DIR_SUFFIX).o

# 6. Build Targets
TARGETS = libamdgpu.so rmapi_server rmapi_client_demo

ifeq ($(OS),haiku)
  TARGETS += amdgpu_hit amdgpu_hit.accelerant
endif

all: $(TARGETS)

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c $< -o $@

libamdgpu.so: $(SRC_OBJS) $(OS_OBJS)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

rmapi_server: $(SRC_DIR)/rmapi_server.o $(SRC_DIR)/hal.o $(SRC_DIR)/objgpu.o \
               $(SRC_DIR)/rmapi.o $(SRC_DIR)/resserv.o $(COMMON_DIR)/ipc_lib.o $(OS_OBJS)
	$(CC) $(CFLAGS) -Wall $^ $(PTHREAD_LIBS) $(LDFLAGS) -o $@

rmapi_client_demo: rmapi_client_demo.c $(filter-out $(SRC_DIR)/rmapi_server.o, $(SRC_OBJS)) $(OS_OBJS)
	$(CC) $(CFLAGS) -Wall $^ $(PTHREAD_LIBS) $(LDFLAGS) -o $@

# --- Haiku Specific Specialist Binaries ---
amdgpu_hit: haiku-amd/addon/AmdAddon.o $(OS_OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

amdgpu_hit.accelerant: haiku-amd/accelerant/AmdAccelerant.o $(COMMON_DIR)/ipc_lib.o $(OS_OBJS)
	$(CXX) -shared -o $@ $^ $(LDFLAGS) $(HAIKU_LDFLAGS)

clean:
	rm -f *.o *.so *.ko $(SRC_DIR)/*.o $(COMMON_DIR)/*.o \
	kernel-amd/os-interface/*/*.o kernel-amd/os-primitives/*/*.o \
	haiku-amd/addon/*.o haiku-amd/accelerant/*.o \
	rmapi_server rmapi_client_demo amdgpu_hit amdgpu_hit.accelerant

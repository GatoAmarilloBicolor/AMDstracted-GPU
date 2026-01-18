# 🏗️ HIT Haiku Driver - Implementation Roadmap & Architecture Analysis

**Status**: Phase 2 (Stub Implementation → Full Feature Development)  
**Last Updated**: Jan 17 2026  
**Team**: Haiku Imposible Team (HIT)

---

## 📋 Executive Summary

The driver has a **solid architecture foundation** but **21+ critical stubs** blocking full functionality:
- **Accelerators**: 2D/3D engine acquisition (stub tokens)
- **Display**: No mode-setting, minimal pixel drawing
- **Device I/O**: No ioctl handling
- **OpenGL/Zink**: Not integrated

This roadmap prioritizes **complete implementation** for Haiku 2D/3D graphics support.

---

## 🏛️ Architecture Overview

### Layer Stack (Bottom → Top)

```
┌─────────────────────────────────────────────┐
│  Haiku App/UI Framework                      │  User Space
├─────────────────────────────────────────────┤
│  Accelerant (Display + 2D/3D)               │
│  ├─ AmdAccelerant.cpp (mode set, FB)       │
│  └─ Engine Hooks (2D/3D acquire/release)    │
├─────────────────────────────────────────────┤
│  RMAPI Layer (High-level GPU Commands)      │
│  ├─ Memory allocation (rmapi_alloc_memory)  │
│  ├─ Buffer submission                        │
│  └─ Synchronization                          │
├─────────────────────────────────────────────┤
│  HAL (Hardware Abstraction Layer)            │
│  ├─ IP Block Management (GMC, GFX, etc)     │
│  ├─ Ring submission                          │
│  └─ Command execution                        │
├─────────────────────────────────────────────┤
│  OS Interface Layer (Haiku-specific)         │
│  ├─ PCI discovery & BAR mapping             │
│  ├─ Display initialization                   │
│  ├─ Interrupt handling                       │
│  └─ Memory allocation (malloc)               │
├─────────────────────────────────────────────┤
│  Kernel Addon (amdgpu_hit device driver)    │  Kernel Space
│  ├─ Device open/close/ioctl                 │
│  ├─ IRQ registration                         │
│  └─ Device publishing (/dev/graphics/...)    │
└─────────────────────────────────────────────┘
```

### IPC Communication Layer

```
┌─────────────────────────────────────────────┐
│  Accelerant (User-space daemon)              │
│         ↓ (IPC via HIT_SOCKET_PATH)         │
│  RMAPI Server (rmapi_server binary)          │
│         ↓ (Shared memory + commands)         │
│  HAL + IP Blocks (Actual GPU control)       │
└─────────────────────────────────────────────┘
```

---

## 📊 Current Implementation Status

### ✅ **WORKING COMPONENTS**

| Component | Status | Details |
|-----------|--------|---------|
| **PCI Discovery** | ✅ | Finds AMD devices on bus |
| **IPC Communication** | ✅ | Socket-based server/client messaging |
| **GPU Info Query** | ✅ | Retrieves VRAM size, clock speed |
| **Safe Display Modes** | ✅ | Hardcoded 640x480 to 1920x1080 |
| **Frame Buffer Config** | ✅ | Returns VRAM base address |
| **Basic Logging** | ✅ | os_prim_log() across OS |
| **Memory Allocation** | ⚠️ | Basic malloc, no GPU-side allocation |

### ❌ **BROKEN/MISSING COMPONENTS**

| Component | Status | Impact | Lines of Code |
|-----------|--------|--------|----------------|
| **2D Engine Acquisition** | ❌ STUB | Cannot draw 2D graphics | AmdAccelerant.cpp:109-118 |
| **Display Mode-Setting** | ❌ STUB | Screen stays blank | AmdAccelerant.cpp:187-193 |
| **OpenGL/Zink Integration** | ❌ STUB | No 3D support | AmdAccelerant.cpp:206-221 |
| **Device ioctl Handler** | ❌ STUB | Cannot communicate with driver | addon/AmdAddon.cpp:42-45 |
| **CRTC Control** | ❌ MISSING | Cannot configure display timing | os_interface_haiku.c |
| **Ring Submission** | ⚠️ PARTIAL | Commands may not execute | hal.c |
| **GPU Memory Fencing** | ❌ STUB | Synchronization issues | rmapi.c |
| **Cursor Management** | ❌ MISSING | No cursor support | - |
| **Hardware Interrupts** | ⚠️ MINIMAL | No IRQ handling for sync | addon/AmdAddon.cpp:42 |

---

## 🎯 Implementation Roadmap (Phase 2-4)

### **PHASE 2: Display Stack (2-3 days)**

#### **2.1 Mode-Setting Pipeline**
```
Goal: Screen displays actual image from framebuffer

Files to modify:
  - AmdAccelerant.cpp: SetDisplayMode() → real implementation
  - os_interface_haiku.c: Add os_display_set_mode()
  - addon/AmdAddon.cpp: Add ioctl dispatcher

Key Tasks:
  1. Parse display_mode request
  2. Calculate CRTC timing parameters
  3. Submit to GPU via RMAPI
  4. Update scanout address
```

**Stubs to fill:**
- `AmdAccelerant::SetDisplayMode()` (line 187)
- `os_display_init()` (line 48) → full CRTC init
- `os_display_put_pixel()` (line 50) → use framebuffer

#### **2.2 CRTC Control Functions**
```c
// Add to os_interface_haiku.c:

typedef struct {
    uint32_t h_total, h_blank, h_front_porch;
    uint32_t v_total, v_blank, v_front_porch;
    uint32_t pixel_clock;
    uint32_t flags;  // polarity, interlace
} crtc_timing;

int os_display_set_crtc_timing(int crtc_id, const crtc_timing *timing);
int os_display_set_scanout_address(int crtc_id, uint64_t address);
int os_display_enable_crtc(int crtc_id, bool enable);
```

**Files to create:**
- `src/os/haiku/display_haiku.c` - CRTC initialization & control

#### **2.3 Framebuffer Memory Mapping**
```
Current: Hardcoded bytes_per_row = 1024 * 4
Target: Calculate from display_mode width/height/color_space

Implementation:
  - Extract pixel format from color_space (B_CMAP8, B_RGB32, etc)
  - Calculate bytes_per_row dynamically
  - Align to GPU requirements (e.g., 256-byte boundaries)
```

---

### **PHASE 3: 2D Acceleration Engine (2-3 days)**

#### **3.1 Engine Token Management**
```
Current Stub:
  amd_acquire_engine() → returns 0x1 (hardcoded token)
  amd_release_engine() → does nothing

Target: Real engine state machine

Files:
  - src/amd/engine_manager.c (NEW)
  - AmdAccelerant.cpp: hook implementations
```

**Implementation Strategy:**
```c
// Engine state management
typedef struct {
    uint32_t token;           // Unique ID
    uint32_t owner_pid;       // Process that owns it
    sync_token *pending_sync; // Wait for previous work
    uint64_t last_fence;      // Last completed fence value
} gpu_engine_state;

// Engine pool
static gpu_engine_state engines[8];  // 8 concurrent engines
static uint32_t next_token = 0x100;

status_t amd_acquire_engine(uint32 caps, uint32 max_wait, 
                           sync_token *st, engine_token **et) {
    // 1. Find free engine matching capabilities
    // 2. Wait for previous owner to finish (respecting max_wait)
    // 3. Issue new fence value
    // 4. Return token
}
```

#### **3.2 2D Acceleration Commands**
```c
// Add to RMAPI layer:

// Copy rectangle (screen blits)
int rmapi_2d_blit(struct OBJGPU *gpu, uint32_t engine_token,
                  const blit_params *params, uint64_t *fence);

// Fill rectangle (needed for clearing)
int rmapi_2d_fill(struct OBJGPU *gpu, uint32_t engine_token,
                  const fill_params *params, uint64_t *fence);

// Wait for completion
int rmapi_wait_fence(struct OBJGPU *gpu, uint64_t fence, uint32_t timeout_ms);
```

**Haiku Accelerant Hooks:**
```c
// In get_accelerant_hook():
case B_SCREEN_TO_SCREEN_BLIT:
    return (void *)amd_screen_to_screen_blit;
case B_FILL_RECTANGLE:
    return (void *)amd_fill_rectangle;
case B_WAIT_ENGINE_IDLE:
    return (void *)amd_wait_engine_idle;
```

---

### **PHASE 4: 3D Graphics & Zink/Vulkan (4-5 days)**

#### **4.1 Zink Integration (OpenGL → Vulkan)**
```
Architecture:
  Haiku BGL App
       ↓ (OpenGL calls)
  Mesa Zink (translates GL → Vulkan)
       ↓ (Vulkan commands)
  RADV (AMD Vulkan driver)
       ↓ (HW commands)
  Our RMAPI + HAL

Task: Connect RADV to our command submission
```

**Files to create:**
- `src/amd/zink_interface.c` - RADV ↔ RMAPI bridge
- `src/os/haiku/vulkan_haiku.c` - Vulkan support initialization

#### **4.2 Command Submission Pipeline**
```c
// In HAL (src/amd/hal/):
// Currently: gmc_v10.c, gfx_v10.c are stubs

// GFX Ring (Graphics Command Ring):
typedef struct {
    uint64_t ring_address;      // GPU memory for commands
    uint32_t ring_size;         // Size in dwords
    uint32_t wptr;              // Write pointer (CPU advances)
    uint32_t rptr;              // Read pointer (GPU advances)
} gfx_ring;

int gfx_v10_ring_init(struct OBJGPU *adev, gfx_ring *ring);
int gfx_v10_ring_commit(struct OBJGPU *adev, const gfx_ring *ring);
```

#### **4.3 Vulkan Instance Management**
```c
// High-level Vulkan coordination:
typedef struct {
    uint32_t min_api_version;
    uint32_t engine_count;
    const char **extensions;
    uint32_t (*get_device_info)(void);
} amd_vulkan_info;

int rmapi_vulkan_init(struct OBJGPU *gpu, amd_vulkan_info *info);
int rmapi_vulkan_create_surface(struct OBJGPU *gpu, 
                                const vulkan_surface_params *params,
                                uint64_t *surface_handle);
```

---

## 🔧 Detailed Component Analysis

### **1. Accelerant Layer** (`src/os/haiku/accelerant/AmdAccelerant.cpp`)

**Purpose**: Haiku's standard graphics interface for display + 2D/3D

**Current State**:
- ✅ Connects to RMAPI server via IPC
- ✅ Caches GPU info (VRAM, clock)
- ✅ Lists safe display modes
- ❌ **SetDisplayMode()** sends request but doesn't wait for response
- ❌ **Engine hooks** return fake tokens
- ❌ **OpenGL hooks** completely stubbed

**What Needs Implementation**:

| Function | Current | Target | Complexity |
|----------|---------|--------|------------|
| `SetDisplayMode` | Stub | Send+wait for RMAPI response | Medium |
| `GetFrameBufferConfig` | Hardcoded FBR | Calculate from mode | Low |
| `AcquireEngine` | Fake token 0x1 | Real state machine | High |
| `ReleaseEngine` | Empty | Cleanup, fence wait | Medium |
| `GetGLRenderer` | Empty | Initialize Zink | Very High |

**IPC Message Flow for Mode-Setting**:
```
1. Accelerant calls SetDisplayMode(1920x1080 @ 60Hz)
2. Sends IPC: {IPC_REQ_SET_DISPLAY_MODE, mode_data}
3. Waits for: {IPC_REP_SET_DISPLAY_MODE, success/error}
4. Meanwhile, RMAPI server:
   - Talks to HAL
   - GMC programs memory
   - GFX sets up scanout timing
   - Returns response
```

---

### **2. RMAPI Layer** (`src/amd/rmapi/rmapi.c` & `rmapi_server.c`)

**Purpose**: High-level GPU command interface (memory, rings, fences)

**Current State**:
- ✅ Global GPU object initialization
- ✅ Memory allocation entry point
- ❌ No actual GPU memory allocation
- ❌ No ring submission
- ❌ No synchronization

**Missing IPC Message Types**:
```c
// Add to ipc_protocol.h:
#define IPC_REQ_SET_DISPLAY_MODE      0x10
#define IPC_REP_SET_DISPLAY_MODE      0x11
#define IPC_REQ_SUBMIT_COMMANDS       0x12
#define IPC_REP_SUBMIT_COMMANDS       0x13
#define IPC_REQ_WAIT_FENCE            0x14
#define IPC_REP_WAIT_FENCE            0x15
#define IPC_REQ_ACQUIRE_ENGINE        0x16
#define IPC_REP_ACQUIRE_ENGINE        0x17
```

---

### **3. HAL Layer** (`src/amd/hal/hal.c` + IP blocks)

**Purpose**: Direct hardware control (MMIO, rings, GPU state)

**Current State**:
- ✅ IP Block registration system
- ⚠️ Basic initialization (hw_init just logs)
- ❌ **gmc_v10.c**: Memory controller stubs (no real MMIO)
- ❌ **gfx_v10.c**: Graphics engine stubs (no command ring)

**What Each IP Block Needs**:

#### **GMC (Memory Controller) - gmc_v10.c**
```c
// Current: Just logs "initializing"
// Needs:
//  - Map GPU memory addresses
//  - Set up page tables
//  - Configure memory bandwidth

typedef struct {
    uint64_t vm_base;        // Virtual memory base
    uint32_t page_table_size;
    uint64_t gart_base;      // Graphics Address Remapping Table
} gmc_config;

int gmc_v10_init_vm(struct OBJGPU *adev, gmc_config *cfg);
int gmc_v10_set_page_table_entry(struct OBJGPU *adev, uint32_t idx, 
                                 uint64_t physical_addr);
```

#### **GFX (Graphics Engine) - gfx_v10.c**
```c
// Current: Just logs "initializing"
// Needs:
//  - Initialize command ring (CP)
//  - Set up graphics pipeline state
//  - Configure wave occupancy

typedef struct {
    uint64_t ring_base;      // Command ring memory
    uint32_t ring_size;      // In dwords
    uint32_t ib_size;        // Indirect buffer size
} gfx_config;

int gfx_v10_init_rings(struct OBJGPU *adev, gfx_config *cfg);
int gfx_v10_submit_ib(struct OBJGPU *adev, const amdgpu_ib *ib);
```

---

### **4. Kernel Addon** (`src/os/haiku/addon/AmdAddon.cpp`)

**Purpose**: Bridge between user-space and kernel (device driver)

**Current State**:
- ✅ Device publishing (/dev/graphics/amdgpu_hit)
- ❌ **device_control()** rejects all ioctls

**What Needs Implementation**:
```c
// Ioctl command handler
typedef struct {
    uint32_t cmd;      // IOCTL code
    void *input;       // Input buffer
    size_t input_size;
    void *output;      // Output buffer
    size_t output_size;
} ioctl_request;

// Add these ioctls:
#define AMDGPU_IOCTL_GET_GPU_INFO     _IOR('A', 0x01, gpu_info_t)
#define AMDGPU_IOCTL_SET_DISPLAY_MODE _IOW('A', 0x02, display_mode_t)
#define AMDGPU_IOCTL_SUBMIT_RING      _IOW('A', 0x03, ring_submit_t)
#define AMDGPU_IOCTL_WAIT_FENCE       _IOW('A', 0x04, fence_wait_t)
```

**Implementation pattern**:
```c
status_t device_control(void *cookie, uint32 op, void *arg, size_t length) {
    switch (op) {
        case AMDGPU_IOCTL_GET_GPU_INFO:
            return amdgpu_ioctl_get_gpu_info((gpu_info_t *)arg);
        case AMDGPU_IOCTL_SET_DISPLAY_MODE:
            return amdgpu_ioctl_set_display_mode((display_mode_t *)arg);
        // ... more ioctls
        default:
            return B_DEV_INVALID_IOCTL;
    }
}
```

---

### **5. Display/CRTC Control** (NEW - `src/os/haiku/display_haiku.c`)

**Purpose**: Manage display outputs (CRTC = CRT Controller)

**What's Needed**:
```c
// CRTC State
typedef struct {
    uint32_t crtc_id;
    bool enabled;
    display_mode current_mode;
    uint64_t scanout_address;
    
    // Timing parameters (from display_mode)
    struct {
        uint16_t h_total, h_active, h_blank_start, h_blank_end;
        uint16_t v_total, v_active, v_blank_start, v_blank_end;
        uint16_t pixel_clock_10khz;
        uint8_t  hsync_polarity, vsync_polarity;
    } timing;
} crtc_state;

// Functions needed:
int display_set_crtc_timing(int crtc_id, const display_mode *mode);
int display_set_scanout(int crtc_id, uint64_t address);
int display_blank_crtc(int crtc_id, bool blank);
int display_wait_vblank(int crtc_id);  // Interrupt-driven
```

---

## 🛠️ Development Guidelines

### **Testing Strategy**

#### **Level 1: Unit Tests (per component)**
```bash
# After each module, run:
make -C src/tests test
```

#### **Level 2: Integration Tests**
```bash
# Test communication chain:
1. Start rmapi_server in background
2. Run rmapi_client_demo
3. Verify IPC messages via /tmp logs
```

#### **Level 3: Display Tests (on Haiku)**
```
1. Boot Haiku
2. Run install_haiku.sh
3. Check /boot/home/config/non-packaged/bin/amd_test_suite
```

### **Code Organization**

```
Each new module should follow pattern:

src/amd/[component]/[component].h
  ├─ Public API (function signatures)
  ├─ Structures
  └─ Constants

src/amd/[component]/[component].c
  ├─ Implementation
  ├─ Static helper functions
  └─ OS abstraction calls
```

### **Logging Strategy**

```c
// Use os_prim_log for all debug output:
os_prim_log("[COMPONENT] Message: %d %s\n", value, string);

// Levels (via AMD_LOG_LEVEL env var):
#define LOG_ERROR   1
#define LOG_INFO    2
#define LOG_DEBUG   3
```

---

## 📈 Complexity Estimates

| Task | Complexity | Est. Hours | Blocking |
|------|-----------|-----------|----------|
| 2.1: Mode-Setting | Medium | 6-8 | YES |
| 2.2: CRTC Control | Medium | 4-6 | YES |
| 2.3: FB Calc | Low | 2 | NO |
| 3.1: Engine Manager | High | 8-12 | YES |
| 3.2: 2D Commands | High | 12-16 | YES |
| 4.1: Zink Bridge | Very High | 16-20 | NO |
| 4.2: Ring Submit | Very High | 20-24 | YES |
| 4.3: Vulkan Init | Very High | 12-16 | NO |

**Total Critical Path**: ~40-50 hours (5-6 days)

---

## 🎓 Haiku/AMDGPU Learning Resources

### **Key Concepts**

1. **Accelerant**: Haiku's abstraction for graphics drivers
   - Hooks-based plugin system
   - Communication via IPC (sockets)
   - Display modes, framebuffer, acceleration hooks

2. **CRTC** (CRT Controller): Display timing generator
   - Horizontal/vertical sync parameters
   - Scanout address (which framebuffer to display)
   - VSYNC interrupts for tearing-free updates

3. **GPU Rings**: Command submission mechanism
   - Ring buffer (circular queue) in GPU memory
   - CPU writes commands, GPU reads and executes
   - Write/Read pointers for synchronization

4. **IPC Communication**: Process-to-process messaging
   - Server: `rmapi_server` (manages GPU)
   - Clients: `Accelerant`, `rmapi_client_demo`
   - Protocol: Message type + serialized data

### **Reference Documentation**

- Haiku Driver Development: `/boot/system/develop/headers/drivers/`
- AMD RDNA ISA: AMDGPU documentation (public)
- Vulkan Spec: https://www.khronos.org/vulkan/

---

## 🚀 Next Steps

1. ✅ **Review this roadmap** with team
2. ⏳ **Start Phase 2.1** (Display Mode-Setting)
3. 🔄 **Weekly progress checkpoints**
4. 📊 **Track stub completion** in STUB_TRACKING.md

---

## 📝 Change Log

- **v1.0** (Jan 17 2026): Initial architecture analysis + roadmap
  - Identified 21+ stubs
  - Prioritized display stack (highest ROI)
  - Mapped IPC message flow
  - Defined component responsibilities

---


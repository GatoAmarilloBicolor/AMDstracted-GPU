# 🔍 Code Analysis: Architecture & Dependencies

**Focus**: Understanding current code structure to implement stubs correctly

---

## 📂 File Structure Deep Dive

### **Layer 1: Accelerant User-Space Driver**

#### `src/os/haiku/accelerant/AmdAccelerant.cpp` (261 lines)

**Architecture**:
```cpp
┌─ AmdAccelerant class (C++ wrapper)
│  ├─ m_conn: IPC connection to rmapi_server
│  ├─ m_gpu_info: Cached GPU capabilities
│  └─ m_connected: Connection status
│
└─ Global instance: g_acc (singleton)
   └─ Haiku hooks → static wrappers → g_acc methods
```

**Critical Functions Analysis**:

| Function | Lines | Current | Problem | Fix |
|----------|-------|---------|---------|-----|
| `Init()` | 120-142 | Connects IPC | Doesn't handle reconnection | Add retry logic |
| `GetDeviceInfo()` | 162-173 | Returns cached info | ✅ WORKING | - |
| `SetDisplayMode()` | 187-193 | Sends request, returns B_OK | **Doesn't wait for response** | Add IPC reply wait |
| `GetFrameBufferConfig()` | 195-204 | Hardcoded bytes_per_row | **1024*4 wrong for other modes** | Calculate from mode |
| `AcquireEngine()` | 109-114 | Returns 0x1 token | **Fake - no state tracking** | Implement state machine |
| `GetGLRenderer()` | 207-212 | Stub comment "For now" | **Returns B_OK, no impl** | Integrate Zink |

**Key Issue - IPC Request/Response Pattern**:
```cpp
// CURRENT (BROKEN):
status_t AmdAccelerant::SetDisplayMode(display_mode *mode) {
    // Only sends request, doesn't wait!
    ipc_message_t msg = {IPC_REQ_SET_DISPLAY_MODE, ...};
    ipc_send_message(&m_conn, &msg);  // ← Fire and forget!
    return B_OK;  // ← Returns immediately
}

// SHOULD BE:
status_t AmdAccelerant::SetDisplayMode(display_mode *mode) {
    ipc_message_t request = {...};
    ipc_send_message(&m_conn, &request);
    
    ipc_message_t reply;
    if (ipc_recv_message(&m_conn, &reply) <= 0)
        return B_ERROR;
    
    if (reply.type != IPC_REP_SET_DISPLAY_MODE)
        return B_ERROR;
    
    // Extract return code from reply.data
    int32_t *result = (int32_t *)reply.data;
    return (*result == 0) ? B_OK : B_ERROR;
}
```

---

### **Layer 2: RMAPI Server (High-Level GPU Interface)**

#### `src/amd/rmapi/rmapi.c` (main GPU coordination)

**Structure**:
```c
┌─ Global state:
│  └─ struct OBJGPU *global_gpu  // Singleton GPU object
│
├─ Public API:
│  ├─ rmapi_init()              // Initialize GPU
│  ├─ rmapi_alloc_memory()      // Allocate GPU memory
│  ├─ rmapi_submit_ring()       // Submit command ring
│  └─ rmapi_wait_fence()        // Synchronization
│
└─ Internal:
   └─ Calls HAL for actual hardware access
```

**Key Functions**:

| Function | Current | Status | Needs |
|----------|---------|--------|-------|
| `rmapi_init()` | Initializes OBJGPU, calls amdgpu_device_init_hal() | ✅ | - |
| `rmapi_alloc_memory()` | Calls amdgpu_buffer_alloc_hal() | ⚠️ STUB | Real GPU memory management |
| `rmapi_submit_ring()` | MISSING | ❌ | Ring submission logic |
| `rmapi_wait_fence()` | MISSING | ❌ | Fence polling/interrupts |

**Memory Allocation Call Chain**:
```
rmapi_alloc_memory()
  ↓
amdgpu_buffer_alloc_hal()  [in hal.c]
  ↓
(Should call GMC to program page tables)
  ↓
Returns physical address to caller
```

**Problem**: `amdgpu_buffer_alloc_hal()` is a stub - it allocates memory but doesn't set up GPU's Memory Controller to see it.

---

#### `src/amd/rmapi/rmapi_server.c` (IPC message handler)

**Architecture**:
```c
// Server loop:
while (running) {
    accept_client_connection()
    
    while (client_connected) {
        msg = receive_ipc_message()
        
        switch (msg.type) {
            case IPC_REQ_GET_GPU_INFO:
                reply = handle_get_gpu_info()
                send_reply()
                break;
            // ... more message types
        }
    }
}
```

**Missing Message Handlers**:
```c
// Currently implemented:
- IPC_REQ_GET_GPU_INFO

// Need to add:
- IPC_REQ_SET_DISPLAY_MODE
  └─ Calls: gmc_set_scanout_address(), gfx_set_crtc_timing()
  
- IPC_REQ_SUBMIT_COMMANDS
  └─ Calls: gfx_submit_ib()
  
- IPC_REQ_WAIT_FENCE
  └─ Calls: rmapi_wait_fence()
  
- IPC_REQ_ACQUIRE_ENGINE
  └─ Returns: unique engine token + fence value
  
- IPC_REQ_ALLOC_MEMORY
  └─ Calls: amdgpu_buffer_alloc_hal() + GMC programming
```

---

### **Layer 3: HAL (Hardware Abstraction)**

#### `src/amd/hal/hal.c` (Master orchestrator)

**Structure**:
```c
┌─ IP Block Registry:
│  ├─ gmc_v10_ip_block     (Memory Controller)
│  ├─ gfx_v10_ip_block     (Graphics Engine)
│  ├─ navi10_common_ip_block (Manager)
│  └─ wrestler_common_ip_block (APU variant)
│
└─ Life Cycle:
   ├─ amdgpu_device_init_hal()
   │  └─ Calls early_init() → hw_init() → late_init() for all blocks
   │
   └─ amdgpu_device_fini_hal()
      └─ Calls hw_fini() for all blocks
```

**IP Block System**:
```c
// Each block has standard lifecycle:
typedef struct {
    const char *name;
    int (*early_init)(struct OBJGPU *adev);    // Detect hardware
    int (*sw_init)(struct OBJGPU *adev);       // Allocate SW resources
    int (*hw_init)(struct OBJGPU *adev);       // Program hardware
    int (*late_init)(struct OBJGPU *adev);     // Final setup
    int (*hw_fini)(struct OBJGPU *adev);       // Power down
} amd_ip_funcs;
```

**Critical Issue - All hw_init() Just Log**:
```c
// CURRENT (BROKEN):
static int gmc_v10_hw_init(struct OBJGPU *adev) {
    os_prim_log("HAL: [GMC] Setting up memory controller\n");
    return 0;  // ← Pretends success!
}

// SHOULD DO:
static int gmc_v10_hw_init(struct OBJGPU *adev) {
    // 1. Map GPU BAR (Memory Mapped I/O) to access registers
    // 2. Initialize GART (Graphics Address Remap Table)
    // 3. Set up virtual memory base
    // 4. Configure memory protection
    // 5. Enable memory controller
    
    return register_mmio_writes();
}
```

---

#### `src/amd/ip_blocks/gmc_v10.c` (Memory Controller)

**Current Code** (very minimal):
```c
static int gmc_v10_sw_init(struct OBJGPU *adev) {
    // Just returns 0
}

static int gmc_v10_hw_init(struct OBJGPU *adev) {
    // Hardcoded fake page table base
    uint32_t page_table_base = 0x400000000ULL;  // WRONG: truncated!
    // No actual MMIO register writes
    return 0;
}
```

**What It Should Do**:
```c
// MMIO Register addresses (from RDNA datasheet):
#define mmVM_CONTEXT0_PAGE_TABLE_BASE_ADDR 0x2000
#define mmVM_CONTEXT0_PAGE_TABLE_START_ADDR 0x2001
#define mmMM_INDEX 0x0
#define mmMM_DATA 0x1

// Implementation skeleton:
static int gmc_v10_hw_init(struct OBJGPU *adev) {
    // 1. Create page table in VRAM
    struct amdgpu_buffer pt_buffer;
    if (amdgpu_buffer_alloc_hal(adev, 4096, &pt_buffer) != 0)
        return -1;
    
    // 2. Write to GPU registers to set page table address
    uint64_t pt_addr = (uint64_t)pt_buffer.gpu_addr;
    
    // Access registers via MMIO:
    // Write(mmVM_CONTEXT0_PAGE_TABLE_BASE_ADDR, pt_addr);
    
    // 3. Enable VM (virtual memory)
    // Write(mmVM_CONTEXT0_CNTL, 1);
    
    return 0;
}
```

---

#### `src/amd/ip_blocks/gfx_v10.c` (Graphics Engine)

**Current Code** (more detailed stub):
```c
static int gfx_v10_hw_init(struct OBJGPU *adev) {
    os_prim_log("[GFX] Setting up graphics engine\n");
    
    // Allocate rings (these are WRONG - just logs, no actual allocation)
    struct amdgpu_ib {
        void *ptr;              // Should be GPU memory
        uint32_t num_dwords;
    };
    // Never actually allocated!
    
    return 0;
}

static int gfx_v10_ring_commit(struct OBJGPU *adev, ...) {
    // Just logs "Submitting to ring"
    // Never actually writes to GPU ring
    return 0;
}
```

**What It Should Do**:
```c
// Ring structure in GPU memory:
typedef struct {
    uint64_t gpu_addr;        // GPU-visible address
    void *cpu_addr;           // CPU-accessible pointer
    uint32_t size_dwords;     // Ring size in 32-bit words
    uint32_t wptr;            // Write pointer (CPU advances)
    uint32_t rptr;            // Read pointer (GPU advances)
} gfx_ring;

static int gfx_v10_hw_init(struct OBJGPU *adev) {
    // 1. Allocate ring buffer (usually 4KB-64KB)
    // 2. Map to both CPU (for writing commands) and GPU (for reading)
    // 3. Initialize ring pointers
    // 4. Program GPU ring registers
    // 5. Enable command processor
    
    return 0;
}

// Command submission:
int gfx_v10_ring_commit(struct OBJGPU *adev, const struct amdgpu_ib *ib) {
    // 1. Copy IB to ring buffer
    // 2. Update write pointer
    // 3. Signal GPU via DOORBELL register
    // 4. Return fence value for tracking completion
    
    return fence_value;
}
```

---

### **Layer 4: OS Interface (Haiku-Specific)**

#### `src/os/haiku/os_interface_haiku.c` (45 lines)

**Current State** - Thin wrappers:
```c
int os_display_init(void) {
    return os_prim_display_init();  // ← Just calls primitive
}

void os_display_put_pixel(int x, int y, uint32_t color) {
    os_prim_display_put_pixel(x, y, color);  // ← Just calls primitive
}
```

**Problem**: These just delegate to `os_prim_*` (OS primitives).

**Should Be**: High-level CRTC control:
```c
// MISSING functions:
typedef struct {
    uint16_t h_active, h_total, h_front_porch, h_sync_width;
    uint16_t v_active, v_total, v_front_porch, v_sync_width;
    uint32_t pixel_clock_10khz;
    uint8_t  hsync_pol, vsync_pol;  // 0=positive, 1=negative
} timing_params;

int os_display_set_crtc_timing(int crtc_id, const timing_params *timing);
int os_display_set_scanout_address(int crtc_id, uint64_t address);
int os_display_enable_crtc(int crtc_id, bool enable);
int os_display_wait_vblank(int crtc_id);
```

---

#### `src/os/haiku/os_primitives_haiku.c` (Actual low-level work)

**Key Functionality**:
```c
// MMIO register access (via /dev/mem):
uint32_t os_prim_mmio_read(uint32_t offset);
void os_prim_mmio_write(uint32_t offset, uint32_t value);

// Memory mapping:
void *os_prim_pci_map_resource(pci_handle, bar_index);

// Simulation fallback (when running on non-AMD):
if (real_hardware_not_available)
    use_malloc_instead_of_real_mmio
```

**Important**: When MMIO unavailable (line 192), falls back to malloc:
```c
// Simulates GPU memory with malloc
#define GPU_VRAM_SIMULATION_SIZE (512 * 1024 * 1024)  // 512MB fake VRAM
static char *simulated_vram = NULL;

void *os_prim_pci_map_resource(...) {
    if (!real_mapping_available) {
        if (!simulated_vram)
            simulated_vram = malloc(GPU_VRAM_SIMULATION_SIZE);
        return simulated_vram;
    }
    // Real hardware path
}
```

---

### **Layer 5: Kernel Addon**

#### `src/os/haiku/addon/AmdAddon.cpp` (61 lines)

**Device Hooks** (Haiku device driver interface):
```cpp
status_t device_open(const char *name, uint32 flags, void **cookie)
  → Haiku calls this when /dev/graphics/amdgpu_hit opened

status_t device_control(void *cookie, uint32 op, void *arg, size_t length)
  → Haiku calls this for ioctl() from user-space
  → Currently: Always returns B_DEV_INVALID_IOCTL ❌

status_t device_read()/device_write()
  → Currently: Reject with B_NOT_ALLOWED ❌
```

**What device_control Needs**:
```cpp
status_t device_control(void *cookie, uint32 op, void *arg, size_t length) {
    switch (op) {
        // GPU discovery
        case AMDGPU_IOCTL_GET_GPU_INFO:
            return copy_gpu_info_to_userspace(arg);
        
        // Display control
        case AMDGPU_IOCTL_SET_DISPLAY_MODE:
            return set_display_mode((display_mode *)arg);
        
        // Command submission
        case AMDGPU_IOCTL_SUBMIT_RING:
            return submit_commands((ring_submit *)arg);
        
        // Synchronization
        case AMDGPU_IOCTL_WAIT_FENCE:
            return wait_fence((fence_wait *)arg);
        
        // Memory management
        case AMDGPU_IOCTL_ALLOC_MEMORY:
            return allocate_gpu_memory((alloc_params *)arg);
        
        default:
            return B_DEV_INVALID_IOCTL;
    }
}
```

---

## 🔗 Data Flow Analysis

### **Scenario 1: Setting Display Mode**

```
User App (Desktop)
    ↓ (SetDisplayMode(1920x1080))
Accelerant (AmdAccelerant.cpp)
    ├─ Validates mode against kSafeModes[]
    ├─ Builds IPC message
    └─ SENDS: IPC_REQ_SET_DISPLAY_MODE
        ↓ (over socket)
RMAPI Server (rmapi_server.c)
    ├─ RECEIVES: message
    ├─ Extracts display_mode from message
    └─ Calls: amdgpu_set_display_mode()
        ↓
HAL (hal.c)
    ├─ GMC block: Programs memory base address
    │   └─ gmc_v10_set_scanout_base()
    │
    └─ GFX block: Programs CRTC timing
        └─ gfx_v10_set_crtc_timing()
            ├─ Calculates H/V sync parameters from mode
            ├─ Writes to MMIO registers (via os_prim_mmio_write)
            └─ Enables CRTC
                
GPU Hardware
    ├─ Memory Controller: Reads from programmed address
    └─ CRTC: Sends H/V sync, reads pixels, displays them
        ↓
Monitor: Image appears! ✅
        
RMAPI Server
    └─ SENDS: IPC_REP_SET_DISPLAY_MODE (success)
        ↓ (over socket)
Accelerant
    └─ RECEIVES: reply
    └─ Returns: B_OK to caller
```

**Critical Issues**:
1. Accelerant doesn't wait for reply (fires and forgets)
2. RMAPI Server: Handler missing entirely
3. GMC: `gmc_v10_set_scanout_base()` is stub
4. GFX: `gfx_v10_set_crtc_timing()` is stub
5. OS Layer: `os_prim_mmio_write()` exists but only simulates

---

### **Scenario 2: 2D Graphics (Rectangle Fill)**

```
App: "Fill 100x100 rectangle with red"
    ↓
Accelerant (AmdAccelerant.cpp)
    ├─ Call: amd_fill_rectangle()  [MISSING]
    ├─ Call: amd_acquire_engine()
    │   └─ Returns: engine_token = 0x1  [FAKE!]
    │
    └─ SENDS: IPC_REQ_2D_FILL
        ├─ fill_params: x, y, width, height, color
        └─ engine_token: 0x1
        
RMAPI Server (rmapi_server.c)
    └─ Handle: IPC_REQ_2D_FILL  [MISSING]
        └─ Call: rmapi_2d_fill()  [MISSING]
        
HAL (hal.c)
    └─ Call: gfx_v10_submit_2d_fill()  [MISSING]
        ├─ Build 2D packet for GPU command ring
        ├─ Submit via ring
        └─ Return fence value
        
GPU Hardware
    ├─ Read 2D packet from ring
    ├─ Execute: Fill rectangle in framebuffer
    └─ Write fence when done
    
Accelerant
    └─ Call: amd_wait_engine_idle(engine_token)  [MISSING]
        └─ Wait for fence from GPU
        └─ Return when complete
```

**Missing Everything**:
- AmdAccelerant: `amd_fill_rectangle()` not implemented
- AmdAccelerant: `amd_wait_engine_idle()` not implemented
- RMAPI: No 2D command handlers
- HAL: No 2D packet generation

---

## 📋 Dependency Graph

```
Accelerant User-Space
├─ Depends on: RMAPI Server (IPC)
│  ├─ Depends on: HAL
│  │  ├─ Depends on: GMC IP block (MMIO)
│  │  ├─ Depends on: GFX IP block (MMIO)
│  │  └─ Depends on: OS Interface
│  │     └─ Depends on: OS Primitives (MMIO, memory)
│  │
│  └─ Depends on: IPC Library
│
└─ Depends on: Kernel Addon (device_control ioctl)
   └─ Depends on: HAL
```

**Build Order for Full Stack**:
1. ✅ OS Primitives (foundation)
2. ✅ IPC Library (communication)
3. ⚠️ HAL (core - needs IP block implementations)
4. ⚠️ RMAPI (depends on HAL)
5. ⚠️ Accelerant (depends on RMAPI)
6. ⚠️ Kernel Addon (optional - can use IPC directly)

---

## 🎯 Stub Implementations Checklist

### **Phase 2: Display Stack**

- [ ] AmdAccelerant::SetDisplayMode() - Add reply wait
- [ ] AmdAccelerant::GetFrameBufferConfig() - Calculate bytes_per_row
- [ ] rmapi_set_display_mode() - RMAPI handler
- [ ] gmc_v10_set_scanout_address() - Program memory base
- [ ] gfx_v10_set_crtc_timing() - Program CRTC registers
- [ ] os_display_set_crtc_timing() - High-level wrapper

### **Phase 3: 2D Acceleration**

- [ ] AmdAccelerant::AcquireEngine() - Real state machine
- [ ] AmdAccelerant::ReleaseEngine() - Cleanup logic
- [ ] amd_fill_rectangle() - 2D fill hook
- [ ] amd_screen_to_screen_blit() - 2D blit hook
- [ ] rmapi_2d_fill() - RMAPI handler
- [ ] rmapi_2d_blit() - RMAPI handler
- [ ] gfx_v10_submit_2d_packet() - GPU command generation

### **Phase 4: 3D/Vulkan**

- [ ] AmdAccelerant::GetGLRenderer() - Zink initialization
- [ ] AmdAccelerant::CreateGLContext() - GL context creation
- [ ] rmapi_3d_submit() - 3D command submission
- [ ] Zink ↔ RADV bridge code

---


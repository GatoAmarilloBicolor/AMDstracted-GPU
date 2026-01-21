# GPU Architecture Comparison - RDNA vs VLIW

**Objective**: Understand the difference between our RDNA driver and the FreeBSD VLIW driver needed for Haiku  

---

## Executive Summary

| Aspect | Our RDNA (Navi10) | FreeBSD VLIW (HD 7290) |
|--------|------------------|----------------------|
| **Target Hardware** | Modern discrete GPU | Old integrated GPU |
| **Architecture** | Modern, simplified | Old, complex |
| **Instruction Format** | Packet-based | 128-bit VLIW bundles |
| **Ring Buffers** | Modern, simple | Classic, complex |
| **Frequency Range** | 1000-2000 MHz | 400-800 MHz |
| **Compute Model** | Wave64 (64 work items) | VLIW4 (4 ops per cycle) |
| **Complexity** | Medium | High |

---

## Architecture Details

### RDNA (Our Current Driver)

```
┌─────────────────────────────────┐
│ Modern Ring-Based (RDNA)         │
├─────────────────────────────────┤
│ Features:                         │
│ ✅ Packet-based commands         │
│ ✅ Simplified instruction set    │
│ ✅ Modern power management       │
│ ✅ Advanced memory VM            │
│ ✅ Complex scheduling            │
│ ✅ High performance              │
└─────────────────────────────────┘

Instruction: Simple packet
├─ Type: 32-bit packet
├─ Header: Type + size
└─ Data: Command payload

Ring: Modern circular queue
├─ Write pointer: CPU updates
├─ Read pointer: GPU updates
└─ Fence: Completion tracking
```

### VLIW (FreeBSD HD 7290)

```
┌─────────────────────────────────┐
│ Classic VLIW-Based              │
├─────────────────────────────────┤
│ Features:                         │
│ ❌ VLIW instruction encoding     │
│ ❌ Complex scheduling            │
│ ✅ Simple memory management      │
│ ⚠️ Legacy power states           │
│ ❌ Multiple ALUs per cycle       │
│ ⚠️ Lower frequency (800MHz max)  │
└─────────────────────────────────┘

Instruction: 128-bit VLIW bundle
├─ ALU0: 32-bit operation
├─ ALU1: 32-bit operation
├─ ALU2: 32-bit operation
└─ ALU3: 32-bit operation

Ring: Classic command buffer
├─ Write pointer: CPU updates
├─ Read pointer: GPU updates
└─ Flags: Status bits (no fence)
```

---

## Command Submission Comparison

### RDNA (Ours) - Simple Packets

```c
// Packet format
struct rdna_packet {
    uint32_t header;      // Type + size
    uint32_t cmd1;        // Command data
    uint32_t cmd2;
    uint32_t cmd3;
};

// Submission
ring_write(ring, packet, 4);  // 4 dwords
ring_doorbell(write_ptr);      // Wake GPU
fence = ring_alloc_fence();    // Track completion
```

### VLIW (FreeBSD) - Complex VLIW Bundles

```c
// VLIW instruction format (128 bits)
struct vliw_instruction {
    uint32_t alu0;        // ALU 0 operation
    uint32_t alu1;        // ALU 1 operation
    uint32_t alu2;        // ALU 2 operation
    uint32_t alu3;        // ALU 3 operation
};

// Complex scheduling needed
// Compiler must arrange 4 ops per cycle
// Dependencies between ALUs
// Register hazards
```

---

## 2D Operations Comparison

### RDNA: Simple 2D Packets

```c
// Build BLIT packet
struct blit_packet {
    uint32_t header;      // BLIT type
    uint32_t src_base;    // Source address
    uint32_t dst_base;    // Dest address
    uint32_t width;       // Size
    uint32_t height;
    uint32_t pitch;       // Bytes per row
};

// Submit: 1 packet = 1 operation
ring_write(ring, packet);
```

### VLIW: Complex VLIW Scheduling

```c
// Build VLIW instructions for BLIT
struct vliw_instruction instructions[8];  // Multiple ops

// Need to schedule:
instructions[0].alu0 = load_src_address();
instructions[0].alu1 = load_pitch();
instructions[0].alu2 = setup_write_buffer();
instructions[0].alu3 = calculate_offsets();

instructions[1].alu0 = iterate_pixels();
instructions[1].alu1 = read_pixel();
instructions[1].alu2 = write_pixel();
instructions[1].alu3 = increment_counters();

// Complex: Dependencies, hazards, stalls
```

---

## Memory Management Comparison

### RDNA: Advanced VM

```c
// Virtual Memory Support
struct vm_context {
    page_table_base;        // Root of page tables
    vm_context_id;          // Per-process ID
    translation_lookaside;  // TLB for translations
};

// Automatic address translation
gpu_addr = vm_translate(cpu_addr);  // Automatic
```

### VLIW: Basic Addressing

```c
// Direct physical addressing mostly
// Minimal VM support
struct phys_address {
    phys_base;              // Physical address
    size;                   // Size
};

// Manual address calculation
gpu_addr = physical_base + offset;  // Manual
```

---

## Ring Buffer Comparison

### RDNA Ring

```
┌─────────────────────────────────┐
│ Write Ptr (CPU) → Ring ← Read Ptr (GPU)
└─────────────────────────────────┘
     ↓
  Fence Value
     ↓
  GPU posts fence #5
     ↓
  CPU polls for fence #5
     ↓
  Work done ✓
```

### VLIW Ring

```
┌─────────────────────────────────┐
│ Write Ptr (CPU) → Ring ← Read Ptr (GPU)
└─────────────────────────────────┘
     ↓
  Status Flags
     ↓
  GPU sets BUSY flag
     ↓
  CPU polls BUSY flag
     ↓
  GPU clears BUSY flag
     ↓
  Work done ✓
```

---

## Performance Characteristics

### RDNA Performance
- **Throughput**: 1-2 TFLOPs (FP32)
- **Frequency**: 1000-2000 MHz
- **Latency**: Low (modern design)
- **Power**: 100-200W

### VLIW Performance
- **Throughput**: 50-100 GFLOPs (4 ops/cycle @ 800MHz)
- **Frequency**: 400-800 MHz
- **Latency**: High (older design)
- **Power**: 5-15W (integrated)

---

## Code Complexity Comparison

### RDNA: Simpler Code

```
File Count: 10-15 files
Lines per file: 200-500 lines
Total: ~3,000 lines
Complexity: Medium (packets, modern features)
```

### VLIW: More Complex Code

```
File Count: 20-30 files
Lines per file: 300-1000 lines  
Total: ~8,000 lines
Complexity: High (VLIW scheduling, legacy code)
```

---

## Which Driver for Haiku?

### Current Situation

**Machine in Meeting (Modern)**:
- Probably has RDNA/NAVI GPU
- Our driver applies directly ✅
- ~2,500 lines implemented
- Performance: Good

**Haiku Machine (Old)**:
- Has HD 7290 (VLIW)
- Needs FreeBSD adaptation
- ~8,000 lines to port
- Performance: Limited but usable

### Recommendation

1. **Our RDNA driver**: Use as-is for modern hardware
2. **FreeBSD VLIW adapter**: Create separate for HD 7290
   - Could share:
     - Display/CRTC layer
     - Accelerant interface
     - OS abstractions
   - Different:
     - GPU command submission
     - 2D/3D operations
     - Power management

---

## Development Path Forward

### Option A: Support Both

```
AMDGPU_Abstracted/
├─ src/amd/hal/
│  ├─ gfx_navi10.c    (RDNA) ✅
│  ├─ gfx_r600.c      (VLIW) ⏳
│  └─ ...
```

Effort: +20-30 hours for VLIW support

### Option B: Focus on RDNA (Modern Hardware)

```
AMDGPU_Abstracted/ (RDNA only)
├─ Current implementation ✅
```

Effort: None additional

### Option C: Separate Projects

```
AMDGPU_Abstracted/ (RDNA)
FreeBSD_Radeon_Adapter/ (VLIW)
```

Effort: Split work, cleaner separation

---

## Conclusion

| Scenario | Driver Choice |
|----------|--------------|
| Modern GPU (RDNA) | ✅ Use current driver |
| Old GPU (VLIW) | ⚠️ Adapt FreeBSD code |
| Both support | 🎯 Modular approach |

The FreeBSD reference files are now available for VLIW architecture study and adaptation.

---

**Last Updated**: Jan 17 2026  
**Status**: Reference complete, ready for VLIW implementation planning


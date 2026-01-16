# RADV Backend

Vulkan device implementation with GPU memory management via GEM allocator.

## Files

- `radv_backend.c` - RADV implementation
- `radv_backend.h` - Public API

## Components

### GEM Allocator
Graphics Execution Model memory allocator
- Tracks up to 256 GPU buffers
- Virtual address management
- 4KB page alignment
- Handle-to-VA mapping

### Command Ring Buffer
64KB circular buffer for GPU commands
- Write and read pointer tracking
- Automatic wrap-around
- 4-byte command alignment

### Device Management
- Physical device enumeration (reports 1 GPU)
- Device properties querying
- Logical device creation
- Memory allocation/mapping

## API

```c
// Initialization
VkResult radv_init(void);
void radv_fini(void);

// Device enumeration
VkResult radv_enumerate_devices(VkPhysicalDevice *devices, 
                               uint32_t *device_count);

// Memory management
VkResult radv_allocate_memory(VkDevice device, size_t size,
                             uint32_t memory_type, VkMemory *memory);
VkResult radv_map_memory(VkDevice device, VkMemory memory, void **ppData);

// Command submission
VkResult radv_queue_submit(VkQueue queue, VkCommandBuffer cmd_buffer);
```

## Memory Layout

```
GPU VA Space
├─ 0x0000_0000 - 0x0FFF_FFFF: Reserved
├─ 0x1000_0000 - 0xFFFF_FFFF: GEM allocations
│  ├─ Buffer 0 @ 0x1000_0000
│  ├─ Buffer 1 @ 0x1000_1000
│  ├─ Buffer 2 @ 0x1001_0000
│  └─ ... (up to 256 buffers)
```

## Status

✅ GEM allocator working  
✅ Ring buffer functional  
✅ Device enumeration complete  
⚠️ No real GPU execution yet (simulation mode)  

## Next Steps

- Real GPU command execution
- Interrupt handling for fences
- Command buffer validation
- Performance optimization

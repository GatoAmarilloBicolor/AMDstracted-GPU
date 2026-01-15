# 🛠 The "How-To": Start Using the GPU!

Ready to build some cool graphics or fast math? This guide will show you how to use this driver without getting a headache.

## 1. Get Ready
You just need a standard computer with `gcc` installed. Whether you're on Linux or Haiku, we've got you covered.

## 2. Make it!
Open your terminal and type:
```bash
make OS=linux  # Change to OS=haiku if that's what you're using!
```
This builds our "Brain" (`rmapi_server`) and a "Test App" (`rmapi_client_demo`).

## 3. Launch the Brain
The driver needs to run in the background like a helpful butler.
```bash
./rmapi_server &
```

## 4. Write your first App
Here is how easy it is to talk to our driver in C:

```c
#include "src/amd/rmapi.h"
#include "src/common/ipc_lib.h"

int main() {
    ipc_connection_t conn;
    
    // 1. Connect to the butler!
    ipc_client_connect("/tmp/amdgpu_rmapi.sock", &conn);
    
    // 2. Ask for some space (Allocate 1MB)
    size_t size = 1024 * 1024;
    ipc_send_message(&conn, &(ipc_message_t){1, 1, sizeof(size), &size});
    
    // 3. Get the answer
    ipc_message_t resp;
    ipc_recv_message(&conn, &resp);
    uint64_t gpu_addr = *(uint64_t*)resp.data;
    
    printf("Yo! I have GPU memory at: 0x%lx\n", gpu_addr);
    
    // 4. Close the connection
    ipc_close(&conn);
    return 0;
}
```

## 🔗 How it compares to NVIDIA
If you've ever used NVIDIA's "RM" driver, our `rmapi_server` is basically the same thing but for AMD.
- Our `rmapi_alloc_memory` is like their `NvRmAllocMemory`.
- Our `rmapi_get_gpu_info` is like their `NV0000_CTRL_GPU_GET_INFO`.

*Have fun building cool stuff!*

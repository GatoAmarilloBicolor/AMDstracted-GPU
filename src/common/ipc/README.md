# IPC (Inter-Process Communication)

Socket-based IPC transport for client-server GPU communication.

## Files

- `ipc_lib.c` - IPC implementation
- `ipc_lib.h` - Public API
- `ipc_protocol.h` - Protocol definitions

## Architecture

```
Client App
    ↓ (UNIX socket)
IPC Connection
    ↓
RMAPI Server
    ↓
GPU Hardware
```

## IPC Message Types

```c
#define IPC_REQ_ALLOC_MEMORY    1   // Allocate GPU memory
#define IPC_REQ_SUBMIT_COMMAND  2   // Submit GPU command
#define IPC_REQ_GET_GPU_INFO    3   // Query GPU properties
```

## API

```c
// Connection management
int ipc_client_connect(const char *socket_path, 
                       ipc_connection_t *conn);
int ipc_close(ipc_connection_t *conn);

// Message passing
int ipc_send_message(ipc_connection_t *conn, 
                    const ipc_message_t *msg);
int ipc_recv_message(ipc_connection_t *conn, 
                    ipc_message_t *msg);
```

## Socket Path

```
/tmp/amdgpu_hit.sock
```

## Protocol

Each message consists of:

```c
typedef struct {
    uint32_t id;          // Client ID
    uint32_t type;        // Message type
    void *data;          // Payload
    size_t data_size;    // Payload size
} ipc_message_t;
```

## Status

✅ Socket communication working  
✅ Message passing functional  
✅ Connection management complete  

## Next Steps

- Add message authentication
- Implement flow control
- Add timeout handling
- Performance optimization

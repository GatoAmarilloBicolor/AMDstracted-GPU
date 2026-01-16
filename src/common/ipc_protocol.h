#ifndef IPC_PROTOCOL_H
#define IPC_PROTOCOL_H

/*
 * 🌀 HIT Edition: The Universal Subway Signal System
 *
 * This file defines the codes our apps and the driver use to talk
 * to each other. Keeping them here means everyone speaks the same language!
 */

// --- Requests (Sent by Apps) ---
#define IPC_REQ_ALLOC_MEMORY 101
#define IPC_REQ_GET_GPU_INFO 102
#define IPC_REQ_FREE_MEMORY 103
#define IPC_REQ_SUBMIT_COMMAND 104

// --- Replies (Sent by Driver) ---
#define IPC_REP_ALLOC_MEMORY 201
#define IPC_REP_GET_GPU_INFO 202
#define IPC_REP_FREE_MEMORY 203
#define IPC_REP_SUBMIT_COMMAND 204

// Standard Socket Path
#define HIT_SOCKET_PATH "/tmp/amdgpu_hit.sock"

#endif

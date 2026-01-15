// RESSERV-like hierarchy implementation, inspired by NVIDIA
// For better resource management in AMD driver

#include "hal.h"
#include "../../kernel-amd/os-primitives/os_primitives.h"
#include <pthread.h>

// Enhanced RESSERV with locking for multi-GPU

struct RsResource* rs_resource_create(uint32_t handle, struct RsResource* parent) {
    struct RsResource* res = os_prim_alloc(sizeof(struct RsResource));
    if (!res) return NULL;
    res->handle = handle;
    res->parent = parent;
    res->children = NULL;
    res->num_children = 0;
    res->data = NULL;
    pthread_mutex_init(&res->lock, NULL);  // Init lock for sync
    os_prim_log("RESSERV: Created resource with lock\n");
    return res;
}

void rs_resource_add_child(struct RsResource* parent, struct RsResource* child) {
    if (!parent || !child) return;
    pthread_mutex_lock(&parent->lock);  // Sync for multi-GPU
    
    struct RsResource** new_children = realloc(parent->children, sizeof(struct RsResource*) * (parent->num_children + 1));
    if (new_children) {
        parent->children = new_children;
        parent->children[parent->num_children++] = child;
        os_prim_log("RESSERV: Added child resource with sync\n");
    } else {
        os_prim_log("RESSERV: Failed to add child resource (OOM)\n");
    }
    
    pthread_mutex_unlock(&parent->lock);
}

void rs_resource_destroy(struct RsResource* res) {
    if (!res) return;
    for (int i = 0; i < res->num_children; i++) {
        rs_resource_destroy(res->children[i]);
    }
    os_prim_free(res->children);
    os_prim_free(res);
    os_prim_log("RESSERV: Destroyed resource\n");
}
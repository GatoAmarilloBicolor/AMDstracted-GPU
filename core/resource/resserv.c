// RESSERV-like hierarchy implementation, inspired by NVIDIA
// For better resource management in AMD driver
// Maintained by: Haiku Imposible Team

#include "../../os/interface/os_primitives.h"
#include "../hal/hal.h"
#include <string.h>

// RESSERV: Resource hierarchy management
// Thread-safety: Use os_prim_lock/unlock for multi-threaded access if needed

#define RS_HASH_SIZE 128
static struct RsResource *rs_hash_table[RS_HASH_SIZE];

static int rs_hash(uint32_t handle) { return handle % RS_HASH_SIZE; }

static void rs_hash_add(struct RsResource *res) {
  /* Lock needed if concurrent access: os_prim_lock(); */
  int idx = rs_hash(res->handle);
  res->hash_next = rs_hash_table[idx];
  rs_hash_table[idx] = res;
  /* Unlock: os_prim_unlock(); */
}

static void rs_hash_remove(struct RsResource *res) {
  /* Lock needed if concurrent access: os_prim_lock(); */
  int idx = rs_hash(res->handle);
  struct RsResource **curr = &rs_hash_table[idx];
  while (*curr) {
    if (*curr == res) {
      *curr = res->hash_next;
      break;
    }
    curr = &((*curr)->hash_next);
  }
  /* Unlock: os_prim_unlock(); */
}

struct RsResource *rs_resource_lookup(uint32_t handle) {
  /* Lock needed if concurrent access: os_prim_lock(); */
  int idx = rs_hash(handle);
  struct RsResource *curr = rs_hash_table[idx];
  while (curr) {
    if (curr->handle == handle) {
      /* Unlock: os_prim_unlock(); */
      return curr;
    }
    curr = curr->hash_next;
  }
  /* Unlock: os_prim_unlock(); */
  return NULL;
}

struct RsResource *rs_resource_create(uint32_t handle,
                                      struct RsResource *parent) {
  struct RsResource *res = os_prim_alloc(sizeof(struct RsResource));
  if (!res)
    return NULL;

  res->handle = handle;
  res->parent = parent;
  res->child_list = NULL;
  res->sibling = NULL;
  res->hash_next = NULL;
  res->data = NULL;
  /* Mutex initialization no longer needed - use os_prim_lock/unlock */

  rs_hash_add(res);

  os_prim_log("RESSERV: Created resource [Handle: 0x%X]\n", handle);
  return res;
}

void rs_resource_add_child(struct RsResource *parent,
                           struct RsResource *child) {
  if (!parent || !child)
    return;
  /* Lock needed if concurrent access: os_prim_lock(); */

  // Link as the new first child (O(1) insertion)
  child->sibling = parent->child_list;
  parent->child_list = child;
  os_prim_log("RESSERV: Linked child 0x%X to parent 0x%X\n", child->handle,
              parent->handle);

  /* Unlock: os_prim_unlock(); */
}

void rs_resource_destroy(struct RsResource *res) {
  if (!res)
    return;

  // 1. Destroy all my children first (Recursion)
  struct RsResource *curr = res->child_list;
  while (curr) {
    struct RsResource *next = curr->sibling;
    rs_resource_destroy(curr);
    curr = next;
  }

  // 2. Remove myself from the global lookup table
  rs_hash_remove(res);

  // 3. Cleanup hardware specialists / data if any
  /* Mutex destroy no longer needed - cleanup via os_prim_cleanup if needed */
  os_prim_free(res);
  os_prim_log("RESSERV: Destroyed resource\n");
}

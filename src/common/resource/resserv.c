// RESSERV-like hierarchy implementation, inspired by NVIDIA
// For better resource management in AMD driver
// Maintained by: Haiku Imposible Team

#include "../../kernel-amd/os-primitives/os_primitives.h"
#include "hal.h"
#include <string.h>

// RESSERV: Simple resource hierarchy (synchronization via os_prim_lock/unlock if needed)

#define RS_HASH_SIZE 128
static struct RsResource *rs_hash_table[RS_HASH_SIZE];
/* Use os_prim_lock/unlock for hash table synchronization when needed */

static int rs_hash(uint32_t handle) { return handle % RS_HASH_SIZE; }

static void rs_hash_add(struct RsResource *res) {
  /* os_prim_lock(); // if multi-threaded access needed */
  int idx = rs_hash(res->handle);
  res->hash_next = rs_hash_table[idx];
  rs_hash_table[idx] = res;
  /* os_prim_unlock(); // if multi-threaded access needed */
}

static void rs_hash_remove(struct RsResource *res) {
  pthread_mutex_lock(&rs_hash_lock);
  int idx = rs_hash(res->handle);
  struct RsResource **curr = &rs_hash_table[idx];
  while (*curr) {
    if (*curr == res) {
      *curr = res->hash_next;
      break;
    }
    curr = &((*curr)->hash_next);
  }
  pthread_mutex_unlock(&rs_hash_lock);
}

struct RsResource *rs_resource_lookup(uint32_t handle) {
  pthread_mutex_lock(&rs_hash_lock);
  int idx = rs_hash(handle);
  struct RsResource *curr = rs_hash_table[idx];
  while (curr) {
    if (curr->handle == handle) {
      pthread_mutex_unlock(&rs_hash_lock);
      return curr;
    }
    curr = curr->hash_next;
  }
  pthread_mutex_unlock(&rs_hash_lock);
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
  pthread_mutex_init(&res->lock, NULL);

  rs_hash_add(res);

  os_prim_log("RESSERV: Created resource [Handle: 0x%X]\n", handle);
  return res;
}

void rs_resource_add_child(struct RsResource *parent,
                           struct RsResource *child) {
  if (!parent || !child)
    return;
  pthread_mutex_lock(&parent->lock);

  // Link as the new first child (O(1) insertion)
  child->sibling = parent->child_list;
  parent->child_list = child;
  os_prim_log("RESSERV: Linked child 0x%X to parent 0x%X\n", child->handle,
              parent->handle);

  pthread_mutex_unlock(&parent->lock);
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
  pthread_mutex_destroy(&res->lock);
  os_prim_free(res);
  os_prim_log("RESSERV: Destroyed resource\n");
}
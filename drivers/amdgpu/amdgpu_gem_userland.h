#ifndef AMDGPU_GEM_USERLAND_H
#define AMDGPU_GEM_USERLAND_H

#include "hal.h"

int amdgpu_gem_create(struct OBJGPU* adev, size_t size, struct amdgpu_buffer* buf);
void amdgpu_gem_destroy(struct OBJGPU* adev, struct amdgpu_buffer* buf);
void* amdgpu_gem_map(struct amdgpu_buffer* buf);
void amdgpu_gem_unmap(struct amdgpu_buffer* buf);

#endif
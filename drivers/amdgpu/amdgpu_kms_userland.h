#ifndef AMDGPU_KMS_USERLAND_H
#define AMDGPU_KMS_USERLAND_H

int amdgpu_kms_setup_display(int width, int height);
void amdgpu_kms_show_framebuffer(const char* ascii_art);
void amdgpu_kms_teardown_display();

#endif
# RMAPI Gallium Target (DRI driver wrapper)
# This creates a DRI driver that wraps RMAPI for OpenGL acceleration

target_name = 'rmapi'

rmapi_dri_sources = files(
  '../rmapi_screen.c',
  '../rmapi_context.c',
  'rmapi_target.c',
  'rmapi_drm.c'
)

rmapi_dri_includes = [
  include_directories('../..'),
  include_directories('../../..'),
  mesa_inc,
]

rmapi_dri_deps = [
  gallium_dep,
  libdrm_dep,
  libdrm_amdgpu_dep,
]

rmapi_dri_libs = [
  libmesa_util,
  libgallium,
  libpipe_loader,
]

rmapi_dri = shared_library(
  'dri_' + target_name,
  rmapi_dri_sources,
  include_directories : rmapi_dri_includes,
  dependencies : rmapi_dri_deps,
  link_with : rmapi_dri_libs,
  link_args : ['-Wl,--as-needed'],
  install : true,
  install_dir : dri_install_dir
)

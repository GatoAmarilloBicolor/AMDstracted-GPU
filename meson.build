project('AMDGPU_Abstracted', 'c', 'cpp',
  version: '0.2.0',
  default_options: ['c_std=c99', 'cpp_std=c++11', 'buildtype=release'])

# Detect OS
host_os = host_machine.system()
if host_os == 'linux'
  os_dir = 'linux'
elif host_os == 'haiku'
  os_dir = 'haiku'
else
  os_dir = 'linux'  # fallback
endif

# Userland mode
userland_mode = get_option('userland_mode')
add_project_arguments('-DUSERLAND_MODE=' + userland_mode.to_string(), language: 'c')

# Includes
incdirs = include_directories('src', 'src/common', 'src/amd', 'src/amd/hal', 'src/amd/rmapi', 'src/os')

# Sources
common_sources = files(
  'src/common/gpu/objgpu.c',
  'src/common/resource/resserv.c',
  'src/common/ipc/ipc_lib.c'
)

amd_sources = files(
  'src/amd/hal/hal.c',
  'src/amd/rmapi/rmapi.c',
  'src/amd/rmapi/rmapi_server.c',
  'src/amd/ip_blocks/gmc_v10.c',
  'src/amd/ip_blocks/gfx_v10.c',
  'src/amd/shader_compiler/shader_compiler.c',
  'src/amd/radv_backend/radv_backend.c',
  'src/amd/zink_layer/zink_layer.c',
  'src/amd/amdgpu_gem_userland.c',
  'src/amd/amdgpu_kms_userland.c'
)

os_sources = files(
  'src/os/@0@/os_primitives_@0@.c'.format(os_dir),
  'src/os/@0@/os_interface_@0@.c'.format(os_dir)
)

# Dependencies
thread_dep = dependency('threads')
math_dep = meson.get_compiler('c').find_library('m', required: false)

# Library
libamdgpu = shared_library('amdgpu',
  sources: common_sources + amd_sources + os_sources,
  include_directories: incdirs,
  dependencies: [thread_dep, math_dep],
  install: true
)

# Executables
rmapi_server = executable('rmapi_server',
  sources: files('src/amd/rmapi/rmapi_server.c'),
  link_with: libamdgpu,
  install: true
)

rmapi_client_demo = executable('rmapi_client_demo',
  sources: files('rmapi_client_demo.c'),
  link_with: libamdgpu,
  install: true
)

# Options
option('userland_mode', type: 'integer', value: 1, description: 'Enable userland mode')
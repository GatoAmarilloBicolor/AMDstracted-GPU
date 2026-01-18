project('AMDGPU_Abstracted', 'c',
  version: '1.0.0',
  default_options: [
    'c_std=c99',
    'warning_level=2',
    'buildtype=debugoptimized'
  ]
)

# Detect OS
host_os = host_machine.system()

# Global includes
global_inc = include_directories('.')

# Core sources
core_sources = files(
  'core/gpu/objgpu.c',
  'core/hal/hal.c',
  'core/resource/resserv.c',
  'core/rmapi/rmapi.c',
  'core/rmapi/rmapi_server.c',
  'core/ipc/ipc_lib.c'
)

# Driver sources
driver_sources = files(
  'drivers/driver_loader.c',
  'drivers/amdgpu/driver_amd.c',
  'drivers/amdgpu/amdgpu_gem_userland.c',
  'drivers/amdgpu/amdgpu_kms_userland.c',
  'drivers/amdgpu/ip_blocks/gmc_v10.c',
  'drivers/amdgpu/ip_blocks/gfx_v10.c',
  'drivers/amdgpu/shader_compiler/shader_compiler.c',
  'drivers/amdgpu/radv_backend/radv_backend.c',
  'drivers/amdgpu/zink_layer/zink_layer.c'
)

# OS-specific sources
if host_os == 'linux'
  os_sources = files(
    'os/linux/os_interface_linux.c',
    'os/linux/os_primitives_linux.c'
  )
  os_inc = include_directories('os/linux')
elif host_os == 'haiku'
  os_sources = files(
    'os/haiku/os_interface_haiku.c',
    'os/haiku/os_primitives_haiku.c'
  )
  os_inc = include_directories('os/haiku')
else
  error('Unsupported OS: ' + host_os)
endif

# All sources
all_sources = core_sources + driver_sources + os_sources

# Includes
inc_dirs = [
  global_inc,
  include_directories('core'),
  include_directories('core/gpu'),
  include_directories('core/hal'),
  include_directories('core/rmapi'),
  include_directories('core/ipc'),
  include_directories('core/resource'),
  include_directories('drivers'),
  include_directories('drivers/interface'),
  include_directories('drivers/amdgpu'),
  include_directories('drivers/amdgpu/ip_blocks'),
  include_directories('drivers/amdgpu/radv_backend'),
  include_directories('drivers/amdgpu/shader_compiler'),
  include_directories('drivers/amdgpu/zink_layer'),
  include_directories('os'),
  include_directories('os/interface'),
  os_inc
]

# Dependencies
deps = [
  dependency('threads'),
  meson.get_compiler('c').find_library('m', required: false)
]

# Library
libamdgpu = shared_library('amdgpu',
  all_sources,
  include_directories: inc_dirs,
  dependencies: deps,
  install: true
)

# Executables
rmapi_server = executable('rmapi_server',
  'core/rmapi/rmapi_server.c',
  include_directories: inc_dirs,
  dependencies: deps,
  link_with: libamdgpu,
  install: true
)

rmapi_client_demo = executable('rmapi_client_demo',
  'examples/rmapi_client_demo.c',
  include_directories: inc_dirs,
  dependencies: deps,
  link_with: libamdgpu,
  install: true
)

# Tests
test_runner = executable('test_runner',
  'src/tests/test_runner.c',
  'src/tests/test_gmc_v10.c',
  include_directories: inc_dirs + [include_directories('src/tests')],
  dependencies: deps,
  link_with: libamdgpu
)

test('gmc_v10_tests', test_runner)

# Install headers
install_headers('os/os_interface.h', subdir: 'amdgpu')
install_headers('drivers/interface/driver_interface.h', subdir: 'amdgpu')
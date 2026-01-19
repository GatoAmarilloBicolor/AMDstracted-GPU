project('AMDGPU_Abstracted', 'c',
  version: '0.9.0',
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

# Core sources (without rmapi_server which has its own main)
core_sources = files(
  'core/gpu/objgpu.c',
  'core/hal/hal.c',
  'core/resource/resserv.c',
  'core/rmapi/rmapi.c',
  'core/ipc/ipc_lib.c'
)

# Server-specific source (has main())
server_sources = files(
  'core/rmapi/rmapi_server.c'
)

# Driver sources (excluding IP blocks for shared library)
driver_sources = files(
  'drivers/driver_loader.c',
  'drivers/interface/mmio_access.c',
  'drivers/interface/drm_access.c',
  'drivers/interface/ring_mgmt.c',
  'drivers/amdgpu/driver_amd.c',
  'drivers/amdgpu/amdgpu_gem_userland.c',
  'drivers/amdgpu/amdgpu_kms_userland.c',
  'drivers/amdgpu/shader_compiler/shader_compiler.c',
  'drivers/amdgpu/radv_backend/radv_backend.c',
  'drivers/amdgpu/zink_layer/zink_layer.c'
)

# IP block sources (separate to avoid duplication in static linking)
ip_block_sources = files(
  'drivers/amdgpu/ip_blocks/gmc_v10.c',
  'drivers/amdgpu/ip_blocks/gfx_v10.c',
  'drivers/amdgpu/ip_blocks/gmc_v10_ip_block.c',
  'drivers/amdgpu/ip_blocks/gfx_v10_ip_block.c',
  'drivers/amdgpu/ip_blocks/dce_v10_ip_block.c',
  'drivers/amdgpu/ip_blocks/dcn_v1_ip_block.c'
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

# OS-specific dependencies and flags
if host_os == 'haiku'
  deps += meson.get_compiler('c').find_library('network', required: true)
  # Add Haiku-specific linker flags for proper ELF headers and compatibility
  add_project_link_arguments([
    '-Wl,--no-undefined',
    '-Wl,-z,defs',
    '-no-pie',  # Disable PIE which can cause loader issues on Haiku
    '-Wl,--hash-style=sysv',  # Use sysv hash for better compatibility
    '-static-libgcc',  # Link libgcc statically to avoid runtime issues
    '-static-libstdc++',  # Link libstdc++ statically for C++ symbols
    '-Wl,--build-id=none'  # Remove build-id which can cause loader issues
  ], language: 'c')
  # Also set compiler flags for Haiku
  add_project_arguments([
    '-D_GNU_SOURCE',  # Enable GNU extensions
    '-fPIC'  # Position independent code
  ], language: 'c')
endif

# All sources combined (for static linking)
all_sources = core_sources + driver_sources + ip_block_sources

# Library - try static linking for Haiku compatibility
if host_os == 'haiku'
  libamdgpu = static_library('amdgpu',
    core_sources + driver_sources + ip_block_sources + os_sources,
    include_directories: inc_dirs,
    dependencies: deps,
    install: false  # Don't install static lib
  )
else
  libamdgpu = shared_library('amdgpu',
    core_sources + driver_sources + ip_block_sources + os_sources,
    include_directories: inc_dirs,
    dependencies: deps,
    install: true,
    install_dir: 'lib'
  )
endif

# Test framework library
test_framework = static_library('test_framework',
  'tests/framework/test_framework.c',
  include_directories: include_directories('tests/framework'),
  dependencies: deps
)

# Executables
if host_os == 'haiku'
  # For Haiku, link statically to avoid runtime loader issues
  rmapi_server = executable('amd_rmapi_server',
    server_sources + all_sources + os_sources,
    include_directories: inc_dirs,
    dependencies: deps,
    link_args: ['-static', '-no-pie'],
    install: false  # Don't auto-install, use script instead
  )

  rmapi_client_demo = executable('amd_rmapi_client_demo',
    'examples/rmapi_client_demo.c',
    all_sources + os_sources,
    include_directories: inc_dirs,
    dependencies: deps,
    link_args: ['-static', '-no-pie'],
    install: false  # Don't auto-install, use script instead
  )
else
  rmapi_server = executable('amd_rmapi_server',
    server_sources,
    include_directories: inc_dirs,
    dependencies: deps,
    link_with: libamdgpu,
    install: true,
    install_dir: 'bin'
  )

  rmapi_client_demo = executable('amd_rmapi_client_demo',
    'examples/rmapi_client_demo.c',
    include_directories: inc_dirs,
    dependencies: deps,
    link_with: libamdgpu,
    install: true,
    install_dir: 'bin'
  )
endif

# Test runner with coverage
if host_os == 'haiku'
  test_runner = executable('amd_test_suite',
    'src/tests/test_runner.c',
    'src/tests/test_gmc_v10.c',
    'tests/mocks/test_mocks.c',
    all_sources + os_sources,
    include_directories: inc_dirs + [include_directories('src/tests'), include_directories('tests'), include_directories('tests/framework')],
    dependencies: deps,
    c_args: ['-fprofile-arcs', '-ftest-coverage'] + get_option('c_args'),
    link_args: ['-static', '-no-pie', '-fprofile-arcs', '-ftest-coverage'] + get_option('c_link_args'),
    install: false  # Don't auto-install, use script instead
  )
else
  test_runner = executable('amd_test_suite',
    'src/tests/test_runner.c',
    'src/tests/test_gmc_v10.c',
    'tests/mocks/test_mocks.c',
    include_directories: inc_dirs + [include_directories('src/tests'), include_directories('tests')],
    dependencies: deps,
    link_with: [libamdgpu, test_framework],
    c_args: ['-fprofile-arcs', '-ftest-coverage'] + get_option('c_args'),
    link_args: ['-fprofile-arcs', '-ftest-coverage'] + get_option('c_link_args'),
    install: true,
    install_dir: 'bin'
  )
endif

# Tests
test('gmc_v10_tests', test_runner)
test('mock_tests', test_runner, args: ['--mocks'])

# Coverage report
gcovr = find_program('gcovr', required: false)
if gcovr.found()
  custom_target('coverage',
    command: [gcovr, '--root', meson.source_root(), '--html', '--html-details', '--output', 'coverage.html'],
    depends: test_runner,
    output: 'coverage.html'
  )
endif
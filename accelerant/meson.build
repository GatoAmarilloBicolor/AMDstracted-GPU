project('amd_gfx', 'c',
	default_options: ['c_std=c99']
)

# AMD Accelerant for Haiku (like nvidia_gsp.accelerant)
# Bridges Haiku Graphics with AMDGPU_Abstracted RMAPI

cc = meson.get_compiler('c')

# Required Haiku libraries
dep_be = cc.find_library('be', required: true)
dep_device = cc.find_library('device', required: false)

# AMDGPU_Abstracted library
subdir('..')
dep_amdgpu = declare_dependency(
	link_with: [libamdgpu],
	include_directories: inc_dirs
)

accelerant_sources = files(
	'src/Accelerant.c',
	'src/HailuAMDInterface.c',
)

# Build AMD graphics accelerant
shared_module('amd_gfx',
	accelerant_sources,
	dependencies: [dep_amdgpu, dep_be],
	name_prefix: '',
	name_suffix: 'accelerant',
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: 'add-ons/accelerants'
)

# Optional: Test utilities
executable('AMD_AccelerantTest',
	'src/AccelerantTest.c',
	dependencies: [dep_be],
	gnu_symbol_visibility: 'hidden',
	install: false
)

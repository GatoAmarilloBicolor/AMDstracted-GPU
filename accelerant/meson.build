project('amd_gfx', 'c',
	default_options: ['c_std=c99']
)

# AMD Accelerant for Haiku (like nvidia_gsp.accelerant)
# Bridges Haiku Graphics with AMDGPU_Abstracted RMAPI

cc = meson.get_compiler('c')

# Detect OS
host_os = host_machine.system()
is_haiku = host_os == 'haiku'

# Required libraries
if is_haiku
	# Haiku-specific libraries
	dep_be = cc.find_library('be', required: true)
	dep_device = cc.find_library('device', required: false)
	
	# Include directories
	inc_amdgpu = include_directories(
		'../src',
		'../core',
		'../drivers',
		'../os'
	)
	
	# AMDGPU libraries (prebuilt or from parent)
	libamdgpu = cc.find_library('amdgpu', required: true)
	
	accelerant_sources = files(
		'src/Accelerant.c',
		'src/HailuAMDInterface.c',
	)
	
	# Build AMD graphics accelerant
	shared_module('amd_gfx',
		accelerant_sources,
		dependencies: [dep_be, libamdgpu],
		include_directories: inc_amdgpu,
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
		include_directories: inc_amdgpu,
		gnu_symbol_visibility: 'hidden',
		install: false
	)
else
	message('Accelerant module requires Haiku OS - skipping accelerant build')
	message('Core AMDGPU_Abstracted libraries are still available')
endif

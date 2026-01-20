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
	
	# AMDGPU library - built in parent directory
	# First check if libamdgpu.a exists in build directory
	libamdgpu_path = run_command('find', '..', '-name', 'libamdgpu.a', '-o', '-name', 'libamdgpu.so', check: false).stdout().strip().split('\n')[0]
	
	if libamdgpu_path == ''
		error('libamdgpu not found. Please build AMDGPU_Abstracted core first.')
	endif
	
	# Create library dependency from found file
	libamdgpu = cc.find_library('amdgpu', dirs: [fs.parent(libamdgpu_path)], required: true)
	
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

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
	# Convert relative paths to absolute for meson find_library
	parent_dir_abs = run_command('realpath', run_command('dirname', meson.current_source_dir(), check: true).stdout().strip(), check: true).stdout().strip()

	lib_search_dirs = [
		parent_dir_abs / 'build.x86_64',
		parent_dir_abs / 'builddir_AMDGPU_Abstracted',
		parent_dir_abs / 'install.x86_64' / 'lib',
	]
	
	libamdgpu = cc.find_library('amdgpu', 
		dirs: lib_search_dirs,
		required: false
	)
	
	if libamdgpu.found()
		message('Found libamdgpu')
	else
		error('libamdgpu not found. Please build AMDGPU_Abstracted core first with: cd .. && ./Build.sh')
	endif
	
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

project('amd_gfx', 'c',
	default_options: ['c_std=c99', 'warning_level=3']
)

# AMD Accelerant for Haiku (like nvidia_gsp.accelerant)
# Platform-agnostic implementation with Haiku adapters
# Bridges Haiku Graphics with AMDGPU_Abstracted RMAPI

cc = meson.get_compiler('c')

# Detect OS
host_os = host_machine.system()
is_haiku = host_os == 'haiku'

# Include directories for platform abstraction
inc_accelerant = include_directories(
	'include',
	'../src',
	'../core',
	'../drivers',
	'../os'
)

# Common sources used on all platforms (for compilation/testing)
common_sources = files(
	'src/Accelerant_v2.c',
)

# Platform-specific configuration
if is_haiku
	# Haiku-specific libraries
	dep_be = cc.find_library('be', required: true)
	dep_device = cc.find_library('device', required: false)
	
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
	
	# Build AMD graphics accelerant for Haiku
	shared_module('amd_gfx',
		common_sources,
		dependencies: [dep_be, libamdgpu],
		include_directories: inc_accelerant,
		c_args: ['-D__HAIKU__'],
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
		include_directories: inc_accelerant,
		c_args: ['-D__HAIKU__'],
		gnu_symbol_visibility: 'hidden',
		install: false
	)
	
	message('Building AMD Accelerant for Haiku OS')
else
	# Non-Haiku platforms: build for compilation testing/validation
	message('Building AMD Accelerant platform abstraction (non-Haiku validation)')
	
	# Create a static library for testing on non-Haiku systems
	static_library('amd_accelerant_core',
		common_sources,
		include_directories: inc_accelerant,
		gnu_symbol_visibility: 'hidden'
	)
	
	# Test executable to validate compilation
	executable('amd_accelerant_test',
		'src/AccelerantTest.c',
		include_directories: inc_accelerant,
		gnu_symbol_visibility: 'hidden',
		install: false
	)
	
	message('Accelerant module compiled for validation on non-Haiku system')
	message('Core AMDGPU_Abstracted libraries are available')
	message('To use AMD accelerant, deploy on Haiku OS')
endif

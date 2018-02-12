lib_files = [ 'ffec.c',
		'ffec_xor.c', 'ffec_encode.c', 'ffec_decode.c', 'ffec_rand.c',
		'ffec_utils.c',
		'ffec_matrix.c' ]



ffec = shared_library(meson.project_name(),
			lib_files,
			include_directories : inc,
			install : true,
			dependencies : deps)
# Make linking work on linux systems without breaking nix
p = get_option('prefix')
if host_machine.system() == 'linux' and not p.startswith('/nix')
	meson.add_install_script('ldconfig', p + '/' + get_option('libdir'))
endif

ffec_static = static_library(meson.project_name(),
			lib_files,
			include_directories : inc,
			install : true,
			dependencies : deps)

# don't set anything here, rely on variables declared in toplevel file
pkg = import('pkgconfig')
pkg.generate(libraries : [ ffec_static, ffec ],
		name : meson.project_name(),
		version : meson.project_version(),
		filebase : meson.project_name(),
		description : description)

# export in case called as a meson subproject
ffec_dep_shared = declare_dependency(link_with : ffec, include_directories : inc)
ffec_dep_static = declare_dependency(link_with : ffec_static, include_directories : inc)

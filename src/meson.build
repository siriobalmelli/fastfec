nonlibc = shared_library(meson.project_name(),
			'fnv.c', 'swap.c',
			include_directories : inc,
			install : true)

# don't set anything here, rely on variables declared in toplevel file
pkg = import('pkgconfig')
pkg.generate(libraries : nonlibc,
		name : meson.project_name(),
		version : meson.project_version(),
		filebase : meson.project_name(),
		description : description)
		

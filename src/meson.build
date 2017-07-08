lib_files = [ 'ffec.c', 'ffec_init_matrix.c', 'ffec_xor.c', 'ffec_matrix.c' ]


# We are being build stand-alone,
#+	so we build shared and generate a pkg-config file.
if not meson.is_subproject()
	ffec = shared_library(meson.project_name(),
				lib_files,
				include_directories : inc,
				install : true,
				dependencies : deps)

	# don't set anything here, rely on variables declared in toplevel file
	pkg = import('pkgconfig')
	pkg.generate(libraries : ffec,
			name : meson.project_name(),
			version : meson.project_version(),
			filebase : meson.project_name(),
			description : description)


# Assume that we export (as a subproject) ONLY a static dependency;
#+	on the rationale that otherwise this library should be compiled and installed
#+	(i.e.: `sudo ninja install` from the 'build-release' dir),
#+	and the superproject will then link to the library using 'pkg-config'.
# If we were to export a shared library, the superproject ends up installing US
#+	when it runs `sudo ninja install`; which is probably not the intended
#	behavior.
else
	ffec = static_library(meson.project_name(),
				lib_files,
				include_directories : inc,
				install : false,
				dependencies : deps)
	ffec_dep = declare_dependency(link_with : ffec,
					include_directories : inc)
endif

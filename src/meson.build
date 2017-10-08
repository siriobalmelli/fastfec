lib_files = [ 'b2hx.c', 'hx2b.c', 'fnv.c', 'npath.c', 'pcg_rand.c', 'lifo.c' ]

lib_files += 'nmem.c'
if host_machine.system() == 'linux'
  lib_files += 'nmem_linux.c'
elif host_machine.system() == 'darwin' or host_machine.system() == 'bsd'
  lib_files += 'nmem_bsd.c'
endif

# Always build both static and shared libraries
nonlibc_shared = shared_library(meson.project_name(),
			lib_files,
			include_directories : inc,
			install : true)
nonlibc_static = static_library(meson.project_name(),
			lib_files,
			include_directories : inc,
			install : false)

# We are being built stand-alone,
#+	so we build shared and generate a pkg-config file.
if not meson.is_subproject()
	nonlibc = nonlibc_shared
	# don't set anything here, rely on variables declared in toplevel file
	pkg = import('pkgconfig')
	pkg.generate(libraries : nonlibc,
			name : meson.project_name(),
			version : meson.project_version(),
			filebase : meson.project_name(),
			description : description)

	if host_machine.system() == 'linux'
		meson.add_install_script('ldconfig', get_option('prefix') + '/' + get_option('libdir'))
	endif

# Assume that we export (as a subproject) ONLY a static dependency;
#+	on the rationale that otherwise this library should be compiled and installed
#+	(i.e.: `sudo ninja install` from the 'build-release' dir),
#+	and the superproject will then link to the library using 'pkg-config'.
# If we were to export a shared library, the superproject ends up installing US
#+	when it runs `sudo ninja install`; which is probably not the intended
#	behavior.
else
	nonlibc = nonlibc_static
	nonlibc_dep = declare_dependency(link_with : nonlibc,
					include_directories : inc)
endif

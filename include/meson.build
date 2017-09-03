# We assume that we will be statically linked if we're a subproject;
#+	ergo: don't pollute the system with our headers
if not meson.is_subproject()
	install_headers('atop.h', 'b2hx.h', 'fnv.h', 'hx2b.h', 'nmath.h', 'nmem.h',
			'nlc_urand.h', 'nonlibc.h', 'npath.h', 'pcg_rand.h',
			'lifo.h', 'zed_dbg.h')
endif

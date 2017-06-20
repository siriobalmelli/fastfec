project('nonlibc', 'c',
	version : '0.0.1',
	license : 'GPL2')
description = 'Collection of standard-not-standard utilities for the discerning C programmer'

add_global_arguments('-std=gnu99', language : 'c')

inc = include_directories('include')
subdir('include')
subdir('src')
subdir('test')

nonlibc_dep = declare_dependency(link_with : nonlibc,
				include_directories : inc)

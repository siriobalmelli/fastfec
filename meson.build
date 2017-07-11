project('nonlibc', 'c',
	version : '0.1.2',
	license : 'GPL2',
	default_options : [ 'c_std=gnu11' ])
description = 'Collection of standard-not-standard utilities for the discerning C programmer'

#sanity
assert(meson.project_name() == 'nonlibc', 'Incorrect project name')

inc = include_directories('include')
subdir('include')
subdir('src')
subdir('test')

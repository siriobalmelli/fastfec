project('nonlibc', 'c',
	version : '0.1.2',
	license : 'GPL2')
description = 'Collection of standard-not-standard utilities for the discerning C programmer'

#sanity
assert(meson.project_name() == 'nonlibc', 'Incorrect project name')

add_project_arguments('-std=gnu99', language : 'c')

inc = include_directories('include')
subdir('include')
subdir('src')
subdir('test')

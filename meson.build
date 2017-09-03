project('nonlibc', 'c',
	version : '0.1.5',
	license : 'GPL2',
	default_options : [ 'c_std=gnu11' ])
description = 'Collection of standard-not-standard utilities for the discerning C programmer'

#sanity
assert(meson.project_name() == 'nonlibc', 'Incorrect project name')

compiler = meson.get_compiler('c')

#
#	build options by type
#
# debug : all the pedantic things!
if get_option('buildtype') == 'debug'
  add_project_arguments([ '-DDEBUG' ],
			language : 'c')

# debugoptimized : daily worker build; basically 'release' but with symbols
elif get_option('buildtype') == 'debugoptimized'
  add_project_arguments([ '-march=native', '-mtune=native', '-fno-omit-frame-pointer' ],
			language : 'c')
  # work around buggy gcc
  if get_option('b_sanitize') != '' and compiler.get_id() == 'gcc'
    message('use gold linker for sanitizers')
    add_project_link_arguments([ '-fuse-ld=gold' ], language : 'c')
  endif

# release : optimize this! ship this!
elif get_option('buildtype') == 'release'
  add_project_arguments([ '-DNDEBUG', '-march=native', '-mtune=native', '-Ofast' ],
			language : 'c')

# plain : NO flags and an old architecture;
#+	use this to cross check the effect of flags on the other build types.
elif get_option('buildtype') == 'plain'
  add_project_arguments([ '-DNDEBUG', '-march=core2' ],
			language : 'c')
endif



inc = include_directories('include')
subdir('include')
subdir('src')
subdir('test')
subdir('util')
subdir('man')

#project
project('ffec', 'c',
	version : '0.0.2',
	default_options : [ 'c_std=gnu11' ])
description = 'Fast FEC library'


#sanity
assert(meson.project_name() == 'ffec', 'Incorrect project name')


#
#	build options by type
#
# debug : all the pedantic things!
if get_option('buildtype') == 'debug'
	add_project_arguments([ '-DDEBUG' ],
				language : 'c')

# debugoptimized : daily worker build; basically 'release' but with symbols
elif get_option('buildtype') == 'debugoptimized'
	add_project_arguments([ '-march=native', '-mtune=native' ],
				language : 'c')

# release : optimize this! ship this!
elif get_option('buildtype') == 'release'
	add_project_arguments([ '-DNDEBUG', '-march=native', '-mtune=native', '-Ofast' ],
				language : 'c')

# plain : NO flags ... use this to cross check the effect of flags on the other build types
elif get_option('buildtype') == 'plain'
	add_project_arguments([ '-DNDEBUG', '-march=core2' ],
				language : 'c')
endif


# deps
nonlibc = dependency('nonlibc', fallback : ['nonlibc', 'nonlibc_dep']) 
crypto = dependency('openssl', required : true) #MD5 in ffec_test
#math is separate only on Linux
cc = meson.get_compiler('c')
m = cc.find_library('m', required : false)
# All deps in a single arg. Use THIS ONE in compile calls
deps = [ nonlibc, m, crypto ]


#build
inc = include_directories('include')
subdir('include')
subdir('src')
subdir('test')

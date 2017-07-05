#project
project('ffec', 'c',
	version : '0.0.2')
description = 'Fast FEC library'


#sanity
assert(meson.project_name() == 'ffec', 'Incorrect project name')

add_project_arguments('-std=gnu99', language : 'c')


# deps
nonlibc = dependency('nonlibc', fallback : ['nonlibc', 'nonlibc_dep']) 
crypto = dependency('openssl', required : true) #TODO: whyyyyyyy
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

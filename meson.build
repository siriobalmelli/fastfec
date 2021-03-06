#project
project('fastfec', 'c',
  version : '0.1.1',
  license : 'LGPL-2.1+',
  default_options : [
    'c_std=gnu11'
    ])
description = 'Fast FEC library'
homepage = 'https://siriobalmelli.github.io/FastFEC/'


# whole-project build flags (most of these are hardening flags)
add_project_arguments([
    '-D_FORTIFY_SOURCE=2',
    '-fasynchronous-unwind-tables',
    '-fexceptions',
    '-fstack-protector-strong',
    '-Werror=format-security',
    '-Werror=implicit-function-declaration'
    ],
  language : 'c')

# _all_ variables visible in gdb and core dumps
if get_option('buildtype') == 'debug'
  add_project_arguments([
    '-DDEBUG',
    '-fno-omit-frame-pointer',
    '-grecord-gcc-switches',
    '-O0',
    '-Wno-cpp'
    ],
    language : 'c')

elif get_option('buildtype') == 'debugoptimized'
  add_project_arguments([
    '-fno-omit-frame-pointer',
    '-grecord-gcc-switches',
    '-march=native',
    '-mtune=native',
    '-O2'
    ],
  language : 'c')

elif get_option('buildtype') == 'release'
  add_project_arguments([
    '-DNDEBUG',
    '-march=native',
    '-mtune=native',
    '-Ofast'
    ],
  language : 'c')

# use this to cross check the effect of flags on the other build types.
elif get_option('buildtype') == 'plain'
  add_project_arguments([
    '-DNDEBUG',
    '-march=core2'
    ],
  language : 'c')
endif



# deps
# deps are either statically or dynamically incorporated;
#+	see meson_options.txt for the default value, which can be overridden
#+	with a -Ddep_type=["shared"||"static"] flag when running meson.

dep_static = false
_dep = get_option('dep_type')
if _dep == 'static'
	dep_static = true
endif

# Insert deps here
# Note that subproject expected to export as '[name]_dep_[shared||static]'
nonlibc = dependency('nonlibc', static: dep_static, version : '>=0.1.8',
		fallback : ['nonlibc', 'nonlibc_dep_' + _dep ], required : true)

#math is separate only on Linux
cc = meson.get_compiler('c')
m = cc.find_library('m', required : false)

# All deps in a single arg. Use THIS ONE in compile calls
deps = [ nonlibc, m ]


#build
inc = include_directories('include')
subdir('include')
subdir('src')
# TODO: replace stupid openssl dependency; re-enable
subdir('test')
subdir('benchmark')


# NOTE: to test different N values; run e.g.:
# `CFLAGS="-DFFEC_N1_DEGREE=7" meson --buildtype release build-release-N7`

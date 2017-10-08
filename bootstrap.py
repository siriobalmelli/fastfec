#!/usr/bin/env python3

import os
from irequire import irequire, run_cmd, templates, targets, which

def do_build(name, path, build):
	run_cmd([ 'ninja', 'test' ], cwd=path, env=build.get('env', {}))
	if build.get('grind', False):
		run_cmd([ 'meson', 'test', '--wrap=valgrind --leak-check=full' ], cwd=path, env={ 'VALGRIND' : '1' })

if __name__ == "__main__":

	global verbose
	verbose = True

	# TODO: replace with loading specific 'templates' and 'target' JSONs
	irequire(templates, targets)

	cscope = which('cscope')
	if cscope:
		run_cmd([ 'cscope', '-b', '-q', '-U', '-I', './include', '-s', './src', '-s', './util', '-s', './test'])

	builds = {
		'debug' :		{ 'type' : 'debug' },
		'release' :		{ 'type' : 'release' },
		'plain' :		{ 'type' : 'plain' },
		'debug-opt' :	{ 'type' : 'debugoptimized', 'grind' : True }
# DO NOT build sanitizers by default.
# Sanitizers place too much of a toolchain burden on the casual library user.
#		'asan' :		{ 'type' : 'debugoptimized', 'opts' : [ '-Db_sanitize=address' ],
#							'env' : { 'VALGRIND' : '1' }
#						},
#		'tsan' :		{ 'type' : 'debugoptimized', 'opts' : [ '-Db_sanitize=thread' ],
#							'env' : { 'VALGRIND' : '1' }
#						}
	}

	for name, build in builds.items():
		path = 'build-%s' % name
		# try and re-build before going whole-hog
		if os.path.isdir(path):
			try:
				do_build(name, path, build)
				continue
			except:
				run_cmd(['rm', '-rfv', path ])
		# heavy-handed approach fixes all the things
		run_cmd([ 'meson' ] + build.get('opts', []) + [ '--buildtype', build['type'], path ])
		do_build(name, path, build)

# TODO : add Windows support

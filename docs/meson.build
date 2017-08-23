# build environment for docs
bundler = find_program('bundle', gempath + 'bundle', required : false)

if bundler.found() == false
	ok = run_command(gem.path(), [ 'install', '--user-install', 'bundler' ])
	if ok.returncode() != 0
		error('cannot install bundle: ' + ok.stderr())
	endif
	bundler = find_program(gempath + 'bundle', required : true)
endif

#docs = run_command(bundler.path(), [ 'exec', 'jekyll', 'build',
#									'--source', '$MESON_SOURCE_ROOT',
#									'--destination', '$MESON_BUILD_ROOT/_site' ])
#if docs.returncode() != 0
#	error('cannot build docs: ' + docs.stderr())
#endif

# build environment for man pages
md2man = find_program('md2man-roff', gempath + 'md2man-roff', required : false)

if md2man.found() == false
	ok = run_command(gem.path(), [ 'install', '--user-install', 'md2man' ])
	if ok.returncode() != 0
		error('cannot install md2man: ' + ok.stderr())
	endif
	md2man = find_program(gempath + 'md2man-roff', required : true)
endif


# build man pages
man_pages = [ 'fnvsum.1' ]

foreach m : man_pages
	one_man = custom_target(' '.join(['man', m]),
			input : '.'.join([m.split('.')[0], 'md']),
			output : m,
			capture : true,
			build_by_default : true,
			command : [ md2man.path(), '@INPUT@' ])
	install_man(one_man.full_path())
endforeach

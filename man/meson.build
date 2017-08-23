man_pages = [ 'fnvsum.1' ]

md2man = find_program('md2man-roff')

foreach m : man_pages
	one_man = custom_target(' '.join(['man', m]),
			input : '.'.join([m.split('.')[0], 'md']),
			output : m,
			capture : true,
			build_by_default : true,
			command : [ md2man.path(), '@INPUT@' ])
	install_man(one_man.full_path())
endforeach

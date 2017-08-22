man_pages = [ 'fnvsum.1' ]

foreach m : man_pages
	one_man = custom_target(' '.join(['man', m]),
			input : '.'.join([m.split('.')[0], 'md']),
			output : m,
			capture : true,
			build_by_default : true,
			command : [ 'md2man-roff', '@INPUT@' ])
	install_man(one_man.full_path())
endforeach

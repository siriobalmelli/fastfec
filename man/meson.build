man_pages = [ 'fnvsum.1' ]

md2man = find_program('md2man-roff')

foreach m : man_pages
	one_man = custom_target(
			' '.join(['man', m]), # e.g.: 'man fnvsum'
			input : '.'.join([m.split('.')[0], 'md']), # e.g.: 'fnvsum.md'
			output : m, # e.g.: 'fnvsum.1'
			capture : true,
			build_by_default : true,
			command : [ md2man, '@INPUT@' ],
			install : true,
			install_dir : join_paths(get_option('mandir'),
									''.join(['man', m.split('.')[1]])) # e.g.: 'man1'
			)
endforeach

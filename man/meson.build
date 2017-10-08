man_pages = [ 'fnvsum.1', 'ncp.1' ]

pandoc = find_program('pandoc')

foreach m : man_pages
  id = m.split('.') #e.g.: [ 'fnvsum', 1 ]
  one_man = custom_target('man ' + id[0],
			input : id[0] + '.md',
			output : m, # e.g.: 'fnvsum.1'
			build_by_default : true,
			install : true,
			command : [ pandoc, '-s', '-t', 'man', '@INPUT@' ],
			capture : true,
			install_dir : join_paths(get_option('mandir'), 'man' + id[1])
			)
endforeach

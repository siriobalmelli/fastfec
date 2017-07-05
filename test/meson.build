ffec_test = executable('ffec_test', 'ffec_test.c',
			include_directories : inc,
			link_with : ffec)
test('ffec test',
	ffec_test,
	args : [ '-f 1.05', '-o 64000000' ],
	is_parallel : false)

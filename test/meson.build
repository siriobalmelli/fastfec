ffec_test = executable('ffec_test', 'ffec_test.c',
			include_directories : inc,
			link_with : ffec,
			dependencies : deps)
test('ffec test',
	ffec_test,
	is_parallel : false)
test('ffec test large',
	ffec_test,
	args : [ '-f 1.05', '-o 128000000' ],
	is_parallel : false)

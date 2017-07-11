ffec_test = executable('ffec_test', 'ffec_test.c',
			include_directories : inc,
			link_with : ffec,
			dependencies : deps)
ffec_test_copy = executable('ffec_test_copy', 'ffec_test.c',
			include_directories : inc,
			link_with : ffec,
			dependencies : deps,
			c_args : [ '-DFFEC_TEST_COPY_SYM' ])
test('ffec test',
	ffec_test)
test('ffec test copy',
	ffec_test_copy)

# more like a benchmark... but OK
test('ffec test large',
	ffec_test,
	args : [ '-f 1.05', '-o 128000000' ],
	is_parallel : false)

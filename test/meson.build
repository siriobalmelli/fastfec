tests = [
	'ffec_test.c'
]

flags = [
	'', '-DFFEC_TEST_COPY_SYM'
]



foreach t : tests
	foreach f : flags
		name = t.split('.')[0] + f
		name_spaced = ' '.join(name.split('_'))

		a_test = executable(name + '_shared', t,
				include_directories : inc,
				link_with : ffec,
				dependencies : [ deps ],
				c_args : [ f ] )
		test(name_spaced + ' (shared)', a_test, timeout : 45,
				args : [ '-f 1.05', '-o 128000000' ])

		test_static = executable(name + '_static', t,
				include_directories : inc,
				link_with : ffec_static,
				dependencies : [ deps ],
				c_args : [ f ] )
		test(name_spaced + ' (static)', test_static, timeout : 45,
				args : [ '-f 1.05', '-o 128000000' ])
	endforeach
endforeach

# more like a benchmark... but OK
#test('ffec test large',
#	ffec_test,
#	args : [ '-f 1.05', '-o 128000000' ],
#	is_parallel : false)

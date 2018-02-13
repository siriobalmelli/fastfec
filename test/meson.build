tests = [
	'ffec_test.c'
]

foreach t : tests
  name = t.split('.')[0]
  name_spaced = ' '.join(name.split('_'))

  a_test = executable(name + '_shared', t,
		      include_directories : inc,
		      link_with : ffec,
		      dependencies : [ deps ])
  test(name_spaced + ' (shared)', a_test, timeout : 45,
		      args : [ '-f 1.05', '-o 128000000' ])

  test_static = executable(name + '_static', t,
		      include_directories : inc,
		      link_with : ffec_static,
		      dependencies : [ deps ])
  test(name_spaced + ' (static)', test_static, timeout : 45,
		      args : [ '-f 1.05', '-o 128000000' ])
endforeach

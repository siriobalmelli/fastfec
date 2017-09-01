tests = [
'atop_test.c',
'fnv_test.c',
'hex2bin2hex_test.c',
'lifo_test.c',
'nlc_urand_test.c',
'nmath_test.c',
'npath_test.c',
'pcg_rand_test.c',
'zed_dbg_test.c' ]

foreach t : tests
	a_test = executable(t.split('.')[0], t,
			include_directories : inc,
			link_with : nonlibc)
	test(' '.join(t.split('.')[0].split('_')), a_test)
endforeach

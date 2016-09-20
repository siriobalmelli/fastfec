
# tests
link_libraries(Judy
	judyutil
	mhash
	)
add_executable(bits_fnv_test.exe bits_fnv_test.c)
add_test(NAME bits_fnv_test COMMAND 
	bits_fnv_test.exe test_lines.txt
	)
set_tests_properties(bits_fnv_test
	PROPERTIES	FAIL_REGULAR_EXPRESSION "err_cnt == [^0]"
	)



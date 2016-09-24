# "bits" library
add_library(bits SHARED bits.c)
target_compile_options(bits PRIVATE -fPIC)

# fnv1a
link_libraries(Judy
	judyutil
	mhash
	)
add_executable(fnv1a_test.exe fnv1a_test.c)
add_test(NAME fnv1a_test COMMAND 
	fnv1a_test.exe test_lines.txt
	)
set_tests_properties(fnv1a_test
	PROPERTIES	FAIL_REGULAR_EXPRESSION "err_cnt == [^0]"
	)

# zed debug
add_executable(zed_dbg_test.exe zed_dbg_test.c)
add_test(NAME zed_dbg_test COMMAND zed_dbg_test.exe)

#ffec library
add_library(ffec SHARED ffec.c ffec_int.c ffec_matrix.c)
target_compile_options(ffec PRIVATE -fPIC -mavx2 -m64)
target_link_libraries(ffec -lm)

link_libraries(ffec)

add_executable(ffec_test.exe ffec_test.c)

add_test(NAME ffec_test COMMAND ffec_test.exe)
set_tests_properties(ffec_test
	PROPERTIES	FAIL_REGULAR_EXPRESSION "err_cnt == [^0]"
	)

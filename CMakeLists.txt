#ffec library
add_library(ffec SHARED ffec.c ffec_int.c ffec_matrix.c)
# TODO: how to compile both debug and production versions of the library?
target_compile_options(ffec PRIVATE -fPIC -mavx2 -m64 -DFFEC_DEBUG)
#target_compile_options(ffec PRIVATE -fPIC -mavx2 -m64)
target_link_libraries(ffec -lm judyutil)


link_libraries(ffec
		fmd5)
add_executable(ffec_test.exe ffec_test.c)

add_test(NAME ffec_test COMMAND ffec_test.exe -f 1.05 -o 64000000)
set_tests_properties(ffec_test
	PROPERTIES	FAIL_REGULAR_EXPRESSION "err_cnt == [^0]"
	)

# "bits" library
add_library(bits SHARED bits.c)
target_compile_options(bits PRIVATE -fPIC)

# zed debug
add_executable(zed_dbg_test.exe zed_dbg_test.c)
add_test(NAME zed_dbg_test COMMAND zed_dbg_test.exe)

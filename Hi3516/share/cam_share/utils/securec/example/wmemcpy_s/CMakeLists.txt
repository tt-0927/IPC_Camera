set (SRC 
 ./wmemcpy_test.c
)
add_executable (wmemcpy_test ${SRC} ${SRC_LIST}) 
target_link_libraries (wmemcpy_test ${LIBS})

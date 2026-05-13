set (SRC 
 ./memcpy_test.c
)
add_executable (memcpy_test ${SRC} ${SRC_LIST}) 
target_link_libraries (memcpy_test ${LIBS})

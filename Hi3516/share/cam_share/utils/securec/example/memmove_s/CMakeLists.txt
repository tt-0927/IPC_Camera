set (SRC 
 ./memmove_test.c
)
add_executable (memmove_test ${SRC} ${SRC_LIST}) 
target_link_libraries (memmove_test ${LIBS})

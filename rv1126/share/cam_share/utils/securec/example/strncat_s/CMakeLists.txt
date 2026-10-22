set (SRC 
 ./strncat_test.c
)
add_executable (strncat_test ${SRC} ${SRC_LIST}) 
target_link_libraries (strncat_test ${LIBS})

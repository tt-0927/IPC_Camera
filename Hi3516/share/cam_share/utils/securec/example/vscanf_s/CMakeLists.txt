set (SRC 
 ./vscanf_test.c
)
add_executable (vscanf_test ${SRC} ${SRC_LIST}) 
target_link_libraries (vscanf_test ${LIBS})

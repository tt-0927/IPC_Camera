set (SRC 
 ./strtok_test.c
)
add_executable (strtok_test ${SRC} ${SRC_LIST}) 
target_link_libraries (strtok_test ${LIBS})

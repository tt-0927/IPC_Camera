set (SRC 
 ./strcat_test.c
)
add_executable (strcat_test ${SRC} ${SRC_LIST}) 
target_link_libraries (strcat_test ${LIBS})

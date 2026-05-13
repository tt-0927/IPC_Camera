set (SRC 
 ./gets_test.c
)
add_executable (gets_test ${SRC} ${SRC_LIST}) 
target_link_libraries (gets_test ${LIBS})

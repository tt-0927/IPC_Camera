set (SRC 
 ./memset_test.c
)
add_executable (memset_test  ${SRC} ${SRC_LIST}) 
target_link_libraries (memset_test ${LIBS})

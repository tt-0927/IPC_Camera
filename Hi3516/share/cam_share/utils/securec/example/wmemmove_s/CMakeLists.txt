set (SRC 
 ./wmemmove_test.c
)
add_executable (wmemmove_test ${SRC} ${SRC_LIST}) 
target_link_libraries (wmemmove_test ${LIBS})

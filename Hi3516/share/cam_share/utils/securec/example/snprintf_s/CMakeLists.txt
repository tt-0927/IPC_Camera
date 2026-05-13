set (SRC 
 ./snprintf_test.c
)
add_executable (snprintf_test ${SRC} ${SRC_LIST}) 
target_link_libraries (snprintf_test ${LIBS})

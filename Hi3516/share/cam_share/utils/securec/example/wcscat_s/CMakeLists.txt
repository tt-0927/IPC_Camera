set (SRC 
 ./wcscat_test.c
)
add_executable (wcscat_test ${SRC} ${SRC_LIST}) 
target_link_libraries (wcscat_test ${LIBS})

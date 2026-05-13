set (SRC 
 ./strcpy_test.c
)
add_executable (strcpy_test ${SRC} ${SRC_LIST}) 
target_link_libraries (strcpy_test ${LIBS})

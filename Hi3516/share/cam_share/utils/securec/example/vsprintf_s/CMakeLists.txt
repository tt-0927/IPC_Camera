set (SRC 
 ./vsprintf_test.c
)
add_executable (vsprintf_test ${SRC} ${SRC_LIST}) 
target_link_libraries (vsprintf_test ${LIBS})

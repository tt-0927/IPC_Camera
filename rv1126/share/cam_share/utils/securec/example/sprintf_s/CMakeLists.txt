set (SRC 
 ./sprintf_test.c
)
add_executable (sprintf_test ${SRC} ${SRC_LIST}) 
target_link_libraries (sprintf_test ${LIBS})

set (SRC 
 ./vsnprintf_test.c
)
add_executable (vsnprintf_test ${SRC} ${SRC_LIST}) 
target_link_libraries (vsnprintf_test ${LIBS})

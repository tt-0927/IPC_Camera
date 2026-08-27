#源文件
set (THREAD_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${THREAD_PATH})
    aux_source_directory (${item} THREAD_LIST)
endforeach()
#头文件
set (THREAD_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${THREAD_INCLUDE})
    include_directories ( ${item} ) 
endforeach()

list(APPEND SRC_LIST ${THREAD_LIST})

#源文件
set (BUFFER_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${BUFFER_PATH})
    aux_source_directory (${item} BUFFER_LIST)
endforeach()
#头文件
set (BUFFER_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${BUFFER_INCLUDE})
    include_directories ( ${item} ) 
endforeach()

list(APPEND SRC_LIST ${BUFFER_LIST})

#源文件
set (RING_BUFFER_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${RING_BUFFER_PATH})
    aux_source_directory (${item} RING_BUFFER_LIST)
endforeach()
#头文件
set (RING_BUFFER_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${RING_BUFFER_INCLUDE})
    include_directories ( ${item} ) 
endforeach()
list(APPEND SRC_LIST ${RING_BUFFER_LIST})

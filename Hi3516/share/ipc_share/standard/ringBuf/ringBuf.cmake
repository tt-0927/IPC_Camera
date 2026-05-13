#源文件
set (RINGBUF_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${RINGBUF_PATH})
    aux_source_directory (${item} RINGBUF_LIST)
endforeach()
#头文件
set (RINGBUF_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${RINGBUF_INCLUDE})
    include_directories ( ${item} ) 
endforeach()
list(APPEND SRC_LIST ${RINGBUF_LIST})

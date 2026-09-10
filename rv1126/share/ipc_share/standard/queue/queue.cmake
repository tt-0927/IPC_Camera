#源文件
set (QUEUE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${QUEUE_PATH})
    aux_source_directory (${item} QUEUE_LIST)
endforeach()
#头文件
set (QUEUE_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${QUEUE_INCLUDE})
    include_directories ( ${item} ) 
endforeach()
list(APPEND SRC_LIST ${QUEUE_LIST})

#源文件
set (THRDPOOL_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${THRDPOOL_PATH})
    aux_source_directory (${item} THRDPOOL_LIST)
endforeach()
#头文件
set (THRDPOOL_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${THRDPOOL_INCLUDE})
    include_directories ( ${item} ) 
endforeach()

list(APPEND SRC_LIST ${THRDPOOL_LIST})

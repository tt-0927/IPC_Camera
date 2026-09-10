#源文件
set (DEBUG_PATH
)
foreach(item ${DEBUG_PATH})
    aux_source_directory (${item} DEBUG_LIST)
endforeach()
#头文件
set (DEBUG_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${DEBUG_INCLUDE})
    include_directories ( ${item} ) 
endforeach()
list(APPEND SRC_LIST ${DEBUG_LIST})

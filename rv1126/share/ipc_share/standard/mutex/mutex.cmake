#源文件
set (MUTEX_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${MUTEX_PATH})
    aux_source_directory (${item} MUTEX_LIST)
endforeach()
#头文件
set (MUTEX_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${MUTEX_INCLUDE})
    include_directories ( ${item} ) 
endforeach()
list(APPEND SRC_LIST ${MUTEX_LIST})

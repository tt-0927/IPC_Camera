#源文件
set (COND_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${COND_PATH})
    aux_source_directory (${item} COND_LIST)
endforeach()
#头文件
set (COND_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${COND_INCLUDE})
    include_directories ( ${item} ) 
endforeach()
list(APPEND SRC_LIST ${COND_LIST})

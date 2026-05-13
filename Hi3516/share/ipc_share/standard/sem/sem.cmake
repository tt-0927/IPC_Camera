#源文件
set (SEM_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${SEM_PATH})
    aux_source_directory (${item} SEM_LIST)
endforeach()
#头文件
set (SEM_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${SEM_INCLUDE})
    include_directories ( ${item} ) 
endforeach()
list(APPEND SRC_LIST ${SEM_LIST})

#源文件
set (LIST_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${LIST_PATH})
    aux_source_directory (${item} LIST_LIST)
endforeach()
#头文件
set (LIST_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${LIST_INCLUDE})
    include_directories ( ${item} ) 
endforeach()
list(APPEND SRC_LIST ${LIST_LIST})

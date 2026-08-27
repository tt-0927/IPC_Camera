#源文件
set (QUE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${QUE_PATH})
    aux_source_directory (${item} QUE_LIST)
endforeach()
#头文件
set (QUE_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${QUE_INCLUDE})
    include_directories ( ${item} ) 
endforeach()
list(APPEND SRC_LIST ${QUE_LIST})

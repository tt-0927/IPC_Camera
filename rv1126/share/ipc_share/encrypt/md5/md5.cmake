#源文件

set (SRC_PATH
    ${CMAKE_CURRENT_LIST_DIR}/
    
)
foreach(item ${SRC_PATH})
    include_directories ( ${item} ) 
    aux_source_directory (${item} MD5_SRC_LIST)
endforeach()

list(APPEND SRC_LIST ${MD5_SRC_LIST} )

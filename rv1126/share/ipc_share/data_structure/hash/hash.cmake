#源文件
set (HASH_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${HASH_PATH})
    aux_source_directory (${item} HASH_LIST)
endforeach()
#头文件
set (HASH_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${HASH_INCLUDE})
    include_directories ( ${item} ) 
endforeach()
list(APPEND SRC_LIST ${HASH_LIST})

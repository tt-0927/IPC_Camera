#源文件
set (HASHCODE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${HASHCODE_PATH})
    aux_source_directory (${item} HASHCODE_LIST)
endforeach()
#头文件
set (HASHCODE_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${HASHCODE_INCLUDE})
    include_directories ( ${item} ) 
endforeach()
list(APPEND SRC_LIST ${HASHCODE_LIST})

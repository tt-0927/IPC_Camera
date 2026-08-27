#源文件
set (ATOM_PATH
)
foreach(item ${ATOM_PATH})
    aux_source_directory (${item} ATOM_LIST)
endforeach()
#头文件
set (ATOM_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${ATOM_INCLUDE})
    include_directories ( ${item} ) 
endforeach()
list(APPEND SRC_LIST ${ATOM_LIST})

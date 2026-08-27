#源文件
set (KERNEL_PATH
)
foreach(item ${KERNEL_PATH})
    aux_source_directory (${item} KERNEL_LIST)
endforeach()
#头文件
set (KERNEL_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${KERNEL_INCLUDE})
    include_directories ( ${item} ) 
endforeach()
list(APPEND SRC_LIST ${KERNEL_LIST})

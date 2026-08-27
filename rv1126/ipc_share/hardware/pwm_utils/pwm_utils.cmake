#源文件
set (PWMTILS_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${PWMTILS_PATH})
    include_directories ( ${item} ) 
    aux_source_directory (${item} PWMUTILS_SRC_LIST)
endforeach()

list(APPEND SRC_LIST ${PWMUTILS_SRC_LIST})
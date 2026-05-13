#源文件
set (GPIOUTILS_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${GPIOUTILS_PATH})
    include_directories ( ${item} ) 
    aux_source_directory (${item} GPIOUTILS_SRC_LIST)
endforeach()

list(APPEND SRC_LIST ${GPIOUTILS_SRC_LIST})
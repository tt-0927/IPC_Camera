#源文件
set (SRC_PATH
    ${CMAKE_CURRENT_LIST_DIR}/
    #${CMAKE_CURRENT_LIST_DIR}/../
)
foreach(item ${SRC_PATH})
    include_directories ( ${item} ) 
    aux_source_directory (${item} HTTP_SRC_LIST)
endforeach()

list(APPEND SRC_LIST ${HTTP_SRC_LIST} )
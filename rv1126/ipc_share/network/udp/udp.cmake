#源文件

set (SRC_PATH
    ${CMAKE_CURRENT_LIST_DIR}/
    ${CMAKE_CURRENT_LIST_DIR}/bl
    ${CMAKE_CURRENT_LIST_DIR}/asio
    ${CMAKE_CURRENT_LIST_DIR}/adapter
    ${CMAKE_CURRENT_LIST_DIR}/../
    ${CMAKE_CURRENT_LIST_DIR}/../../shared_library/asio
)
foreach(item ${SRC_PATH})
    include_directories ( ${item} ) 
    aux_source_directory (${item} UDP_SRC_LIST)
endforeach()

list(APPEND SRC_LIST ${UDP_SRC_LIST} )
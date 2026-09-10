#源文件

include(${CMAKE_CURRENT_LIST_DIR}/../../encrypt/md5/md5.cmake)
set (SRC_PATH
    ${CMAKE_CURRENT_LIST_DIR}/
    ${CMAKE_CURRENT_LIST_DIR}/libwebsockets
    ${CMAKE_CURRENT_LIST_DIR}/../
    ${CMAKE_CURRENT_LIST_DIR}/../../shared_library/websocket
    ${CMAKE_CURRENT_LIST_DIR}/../../shared_library/websocket/libwebsockets
    ${CMAKE_CURRENT_LIST_DIR}/../../shared_library/openssl
)
foreach(item ${SRC_PATH})
    include_directories ( ${item} ) 
    aux_source_directory (${item} WS_SRC_LIST)
endforeach()

list(APPEND SRC_LIST ${WS_SRC_LIST} )

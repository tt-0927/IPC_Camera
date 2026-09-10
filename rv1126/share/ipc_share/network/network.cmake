include(${CMAKE_CURRENT_LIST_DIR}/tcp/tcp.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/udp/udp.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/uds/uds.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/websocket/websocket.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/http/http.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/rtp/rtp.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/mqtt/mqtt.cmake)

#源文件
set (SYS_NETWORK_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${SYS_NETWORK_PATH})
    include_directories ( ${item} ) 
    aux_source_directory (${item} SYS_NETWORK_LIST)
endforeach()

list(APPEND SRC_LIST ${SYS_NETWORK_LIST})

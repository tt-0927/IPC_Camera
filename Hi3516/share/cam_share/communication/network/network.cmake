# network.cmake

# 引用cmake
# include(${CMAKE_CURRENT_LIST_DIR}/curl_http/curl_http.cmake)
# include(${CMAKE_CURRENT_LIST_DIR}/network_libevent/network_libevent.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/sdk_net_base/sdk_net_base.cmake)
# include(${CMAKE_CURRENT_LIST_DIR}/shortLink/shortLink.cmake)
# include(${CMAKE_CURRENT_LIST_DIR}/udp/udp.cmake)
# include(${CMAKE_CURRENT_LIST_DIR}/udp_sdk_base/udp_sdk_base.cmake)

# 头文件
set (INCLUDE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)

# 源文件
set (SOURCE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)

# 添加头文件
foreach(item ${INCLUDE_PATH})
    include_directories (${item}) 
endforeach()

# 添加源文件
foreach(item ${SOURCE_PATH})
    aux_source_directory (${item} SOURCE_LIST)
endforeach()

list(APPEND SRC_LIST ${SOURCE_LIST})
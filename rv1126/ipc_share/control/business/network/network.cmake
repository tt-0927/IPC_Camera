# network.cmake

# 引用
include(${CMAKE_CURRENT_LIST_DIR}/bonjour/bonjour.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/ddns/ddns.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/email/email.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/gm_cert/gm_cert.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/https/https.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/pppoe/pppoe.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/qos/qos.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/snmp/snmp.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/upnp/upnp.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/wifi/wifi.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/4g/4g.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/hostapd/hostapd.cmke)
include(${CMAKE_CURRENT_LIST_DIR}/platform/platform.cmke)

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
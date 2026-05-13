include( ${CMAKE_CURRENT_LIST_DIR}/../../encrypt/gb35114/gb35114.cmake)

#源文件
set (GB28181_PATH
    ${CMAKE_CURRENT_LIST_DIR}/
    ${CMAKE_CURRENT_LIST_DIR}/sip
    ${CMAKE_CURRENT_LIST_DIR}/sip/common
    ${CMAKE_CURRENT_LIST_DIR}/sip/device
    ${CMAKE_CURRENT_LIST_DIR}/sip/event
    ${CMAKE_CURRENT_LIST_DIR}/sip/gm
    ${CMAKE_CURRENT_LIST_DIR}/sip/media
    ${CMAKE_CURRENT_LIST_DIR}/sip/media/libmpeg/source
    ${CMAKE_CURRENT_LIST_DIR}/sip/network
    ${CMAKE_CURRENT_LIST_DIR}/sip/request

)
foreach(item ${GB28181_PATH})
    aux_source_directory (${item} GB28181_LIST)
endforeach()
#头文件
set (GB28181_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}/
    ${CMAKE_CURRENT_LIST_DIR}/sip
    ${CMAKE_CURRENT_LIST_DIR}/sip/common
    ${CMAKE_CURRENT_LIST_DIR}/sip/device
    ${CMAKE_CURRENT_LIST_DIR}/sip/event
    ${CMAKE_CURRENT_LIST_DIR}/sip/gm
    ${CMAKE_CURRENT_LIST_DIR}/sip/media
    ${CMAKE_CURRENT_LIST_DIR}/sip/media/libmpeg/include
    ${CMAKE_CURRENT_LIST_DIR}/sip/network
    ${CMAKE_CURRENT_LIST_DIR}/sip/request
)
foreach(item ${GB28181_INCLUDE})
    include_directories ( ${item} ) 
endforeach()
list(APPEND SRC_LIST ${GB28181_LIST})

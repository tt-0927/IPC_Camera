#源文件
set (ONVIF_PATH
    ${CMAKE_CURRENT_LIST_DIR}/
    ${CMAKE_CURRENT_LIST_DIR}/service
    ${CMAKE_CURRENT_LIST_DIR}/server
    ${CMAKE_CURRENT_LIST_DIR}/common
    ${CMAKE_CURRENT_LIST_DIR}/convert
)
foreach(item ${ONVIF_PATH})
    aux_source_directory (${item} ONVIF_LIST)
endforeach()
#头文件
set (ONVIF_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}/
    ${CMAKE_CURRENT_LIST_DIR}/define
    ${CMAKE_CURRENT_LIST_DIR}/server
    ${CMAKE_CURRENT_LIST_DIR}/common
    ${CMAKE_CURRENT_LIST_DIR}/convert
)
foreach(item ${ONVIF_INCLUDE})
    include_directories ( ${item} ) 
endforeach()
list(APPEND SRC_LIST ${ONVIF_LIST})

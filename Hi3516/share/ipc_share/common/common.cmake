# common.cmake

include(${CMAKE_CURRENT_LIST_DIR}/utils/utils.cmake)

#源文件
set (COMMON_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/config_compat
    ${CMAKE_CURRENT_LIST_DIR}/convert
    ${CMAKE_CURRENT_LIST_DIR}/define
)

foreach(item ${COMMON_PATH})
    aux_source_directory (${item} COMMON_LIST)
endforeach()
#头文件
set (COMMON_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/config_compat
    ${CMAKE_CURRENT_LIST_DIR}/convert
    ${CMAKE_CURRENT_LIST_DIR}/define
)
foreach(item ${COMMON_INCLUDE})
    include_directories ( ${item} ) 
endforeach()
list(APPEND SRC_LIST ${COMMON_LIST})
# peripheral.cmake

# 头文件
set (INCLUDE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/config
    ${CMAKE_CURRENT_LIST_DIR}/fill_light
)

# 源文件
set (SOURCE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/config
    ${CMAKE_CURRENT_LIST_DIR}/fill_light
)

foreach(item ${INCLUDE_PATH})
    include_directories (${item})
endforeach()

set (PERIPHERAL_SOURCE_LIST)
foreach(item ${SOURCE_PATH})
    aux_source_directory (${item} PERIPHERAL_SOURCE_TEMP)
    list(APPEND PERIPHERAL_SOURCE_LIST ${PERIPHERAL_SOURCE_TEMP})
    unset(PERIPHERAL_SOURCE_TEMP)
endforeach()

list(APPEND SRC_LIST ${PERIPHERAL_SOURCE_LIST})

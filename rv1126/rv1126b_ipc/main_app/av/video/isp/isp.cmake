# isp.cmake

# 头文件
set (INCLUDE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/bootstrap
    ${CMAKE_CURRENT_LIST_DIR}/capability
    ${CMAKE_CURRENT_LIST_DIR}/platform
    ${CMAKE_CURRENT_LIST_DIR}/../../../peripheral
)

# 源文件
set (SOURCE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/bootstrap
    ${CMAKE_CURRENT_LIST_DIR}/capability
    ${CMAKE_CURRENT_LIST_DIR}/platform
    ${CMAKE_CURRENT_LIST_DIR}/../../../peripheral
)

# 添加头文件
foreach(item ${INCLUDE_PATH})
    include_directories (${item}) 
endforeach()

# 添加源文件；使用独立临时列表，避免多个目录之间重复累积 aux_source_directory 结果。
set (ISP_SOURCE_LIST)
foreach(item ${SOURCE_PATH})
    aux_source_directory (${item} ISP_SOURCE_TEMP)
    list(APPEND ISP_SOURCE_LIST ${ISP_SOURCE_TEMP})
    unset(ISP_SOURCE_TEMP)
endforeach()

list(APPEND SRC_LIST ${ISP_SOURCE_LIST})

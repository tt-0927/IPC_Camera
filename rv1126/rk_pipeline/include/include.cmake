# include.cmake

# 头文件
set (INCLUDE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/rkaiq/uAPI2
    ${CMAKE_CURRENT_LIST_DIR}/rkaiq
    ${CMAKE_CURRENT_LIST_DIR}/rkaiq/common   
    ${CMAKE_CURRENT_LIST_DIR}/rkaiq/xcore
    ${CMAKE_CURRENT_LIST_DIR}/rkaiq/algos
    ${CMAKE_CURRENT_LIST_DIR}/rkaiq/iq_parser
    ${CMAKE_CURRENT_LIST_DIR}/rkaiq/iq_parser_v2
    ${CMAKE_CURRENT_LIST_DIR}/rkaiq/smartIr
)

# 源文件
set (SOURCE_PATH
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
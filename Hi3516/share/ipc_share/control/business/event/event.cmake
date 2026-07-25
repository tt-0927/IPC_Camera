# event.cmake

# 头文件
set (INCLUDE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/abnormal
    ${CMAKE_CURRENT_LIST_DIR}/ai
    ${CMAKE_CURRENT_LIST_DIR}/config
    ${CMAKE_CURRENT_LIST_DIR}/linkage
    ${CMAKE_CURRENT_LIST_DIR}/linkage/base
    ${CMAKE_CURRENT_LIST_DIR}/linkage/action
    ${CMAKE_CURRENT_LIST_DIR}/monitor
    ${CMAKE_CURRENT_LIST_DIR}/onvif
    ${CMAKE_CURRENT_LIST_DIR}/retrieval
)

# 源文件
set (SOURCE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/abnormal
    ${CMAKE_CURRENT_LIST_DIR}/ai
    ${CMAKE_CURRENT_LIST_DIR}/config
    ${CMAKE_CURRENT_LIST_DIR}/linkage
    ${CMAKE_CURRENT_LIST_DIR}/linkage/base
    ${CMAKE_CURRENT_LIST_DIR}/linkage/action
    ${CMAKE_CURRENT_LIST_DIR}/monitor
    ${CMAKE_CURRENT_LIST_DIR}/onvif
    ${CMAKE_CURRENT_LIST_DIR}/retrieval
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

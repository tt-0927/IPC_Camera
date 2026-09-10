# isp.cmake

# 头文件
set (INCLUDE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/capability
    ${CMAKE_CURRENT_LIST_DIR}/config/application
    ${CMAKE_CURRENT_LIST_DIR}/config/model
    ${CMAKE_CURRENT_LIST_DIR}/config/storage
    ${CMAKE_CURRENT_LIST_DIR}/interface
    ${CMAKE_CURRENT_LIST_DIR}/peripheral
    ${CMAKE_CURRENT_LIST_DIR}/orchestration
    ${CMAKE_CURRENT_LIST_DIR}/orchestration/daynight
    ${CMAKE_CURRENT_LIST_DIR}/orchestration/fill_light
    ${CMAKE_CURRENT_LIST_DIR}/orchestration/param
    ${CMAKE_CURRENT_LIST_DIR}/orchestration/scene
    ${CMAKE_CURRENT_LIST_DIR}/policy
    ${CMAKE_CURRENT_LIST_DIR}/runtime
    ${CMAKE_CURRENT_LIST_DIR}/runtime/model
)

# 源文件
set (SOURCE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/capability
    ${CMAKE_CURRENT_LIST_DIR}/config/application
    ${CMAKE_CURRENT_LIST_DIR}/config/storage
    ${CMAKE_CURRENT_LIST_DIR}/interface
    ${CMAKE_CURRENT_LIST_DIR}/peripheral
    ${CMAKE_CURRENT_LIST_DIR}/orchestration
    ${CMAKE_CURRENT_LIST_DIR}/orchestration/daynight
    ${CMAKE_CURRENT_LIST_DIR}/orchestration/param
    ${CMAKE_CURRENT_LIST_DIR}/orchestration/scene
    ${CMAKE_CURRENT_LIST_DIR}/policy
    ${CMAKE_CURRENT_LIST_DIR}/runtime
)

# 添加头文件
foreach(item ${INCLUDE_PATH})
    include_directories (${item})
endforeach()

# 添加源文件
set (ISP_SOURCE_LIST)
foreach(item ${SOURCE_PATH})
    aux_source_directory (${item} ISP_SOURCE_TEMP)
    list(APPEND ISP_SOURCE_LIST ${ISP_SOURCE_TEMP})
    unset(ISP_SOURCE_TEMP)
endforeach()

list(APPEND SRC_LIST ${ISP_SOURCE_LIST})

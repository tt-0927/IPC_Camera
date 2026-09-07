# pipeline.cmake

# AI 检测结果标准化与事件分发 Pipeline 子模块
# 由 event.cmake 显式 include，嵌套目录不依赖递归扫描

# 头文件
set (AI_PIPELINE_INCLUDE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/core
    ${CMAKE_CURRENT_LIST_DIR}/geometry
    ${CMAKE_CURRENT_LIST_DIR}/output
    ${CMAKE_CURRENT_LIST_DIR}/process
    ${CMAKE_CURRENT_LIST_DIR}/process/people_flow
    ${CMAKE_CURRENT_LIST_DIR}/process/people_flow/state
    ${CMAKE_CURRENT_LIST_DIR}/process/people_density
    ${CMAKE_CURRENT_LIST_DIR}/process/region
    ${CMAKE_CURRENT_LIST_DIR}/process/region/state
    ${CMAKE_CURRENT_LIST_DIR}/process/trip_line
    ${CMAKE_CURRENT_LIST_DIR}/process/trip_line/state
    ${CMAKE_CURRENT_LIST_DIR}/process/dwell
    ${CMAKE_CURRENT_LIST_DIR}/process/dwell/state
    ${CMAKE_CURRENT_LIST_DIR}/process/face
)

# 源文件
set (AI_PIPELINE_SOURCE_PATH
    ${CMAKE_CURRENT_LIST_DIR}/core
    ${CMAKE_CURRENT_LIST_DIR}/geometry
    ${CMAKE_CURRENT_LIST_DIR}/process/people_flow
    ${CMAKE_CURRENT_LIST_DIR}/process/people_flow/state
    ${CMAKE_CURRENT_LIST_DIR}/process/people_density
    ${CMAKE_CURRENT_LIST_DIR}/process/region
    ${CMAKE_CURRENT_LIST_DIR}/process/region/state
    ${CMAKE_CURRENT_LIST_DIR}/process/trip_line
    ${CMAKE_CURRENT_LIST_DIR}/process/trip_line/state
    ${CMAKE_CURRENT_LIST_DIR}/process/dwell
    ${CMAKE_CURRENT_LIST_DIR}/process/dwell/state
    ${CMAKE_CURRENT_LIST_DIR}/process/face
)

# 添加头文件
foreach(item ${AI_PIPELINE_INCLUDE_PATH})
    include_directories (${item})
endforeach()

# 添加源文件
foreach(item ${AI_PIPELINE_SOURCE_PATH})
    aux_source_directory (${item} AI_PIPELINE_SOURCE_LIST)
endforeach()

list(APPEND SRC_LIST ${AI_PIPELINE_SOURCE_LIST})

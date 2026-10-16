# pipeline.cmake

# AI 检测结果标准化与事件分发 Pipeline 子模块
# 由 event.cmake 显式 include，嵌套目录不依赖递归扫描

# 头文件目录：本模块源码经全局 SRC_LIST 并入消费方可执行目标（include 时目标
# 尚未创建，无法 target 化），include_directories 全局注册是现状约定；
# 新增处理器目录须在此追加
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

# 源文件显式清单：不用 aux_source_directory（感知不到新增文件，CMake 官方不推荐）。
# 注意 CAP_UNIFIED_EVENT_PIPELINE=0 时所有文件编译为空，漏登一个 .cpp 只在
# CAP=1 的型号链接期才报 undefined reference，新增源文件必须同步追加
set (AI_PIPELINE_SOURCE_LIST
    ${CMAKE_CURRENT_LIST_DIR}/core/result_dispatcher.cpp
    ${CMAKE_CURRENT_LIST_DIR}/geometry/normalized_geometry.cpp
    ${CMAKE_CURRENT_LIST_DIR}/process/dwell/dwell_config_adapter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/process/dwell/dwell_processor.cpp
    ${CMAKE_CURRENT_LIST_DIR}/process/dwell/state/dwell_track_store.cpp
    ${CMAKE_CURRENT_LIST_DIR}/process/face/face_config_adapter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/process/face/face_processor.cpp
    ${CMAKE_CURRENT_LIST_DIR}/process/people_density/people_density_config_adapter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/process/people_density/people_density_processor.cpp
    ${CMAKE_CURRENT_LIST_DIR}/process/people_flow/people_flow_config_adapter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/process/people_flow/people_flow_processor.cpp
    ${CMAKE_CURRENT_LIST_DIR}/process/people_flow/state/people_flow_statistics_state.cpp
    ${CMAKE_CURRENT_LIST_DIR}/process/people_flow/state/people_flow_track_store.cpp
    ${CMAKE_CURRENT_LIST_DIR}/process/region/region_config_adapter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/process/region/region_processor.cpp
    ${CMAKE_CURRENT_LIST_DIR}/process/region/state/region_track_store.cpp
    ${CMAKE_CURRENT_LIST_DIR}/process/trip_line/state/trip_line_track_store.cpp
    ${CMAKE_CURRENT_LIST_DIR}/process/trip_line/trip_line_config_adapter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/process/trip_line/trip_line_processor.cpp
)

# 配置期存在性校验：清单写错路径立即报错，不留到链接期
foreach(f ${AI_PIPELINE_SOURCE_LIST})
    if(NOT EXISTS ${f})
        message(FATAL_ERROR "event pipeline 源文件不存在：${f}，请核对 AI_PIPELINE_SOURCE_LIST")
    endif()
endforeach()

# 添加头文件（全局注册，见文件头说明）
foreach(item ${AI_PIPELINE_INCLUDE_PATH})
    include_directories (${item})
endforeach()

# 并入消费方全局源列表
list(APPEND SRC_LIST ${AI_PIPELINE_SOURCE_LIST})

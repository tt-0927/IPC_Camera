# event.cmake
#
# AI 检测结果标准化与事件分发 Pipeline 模块入口
# 由 control/business/event/event.cmake 显式 include

# 引入 pipeline 子模块（包含 core/geometry/output/process 等）
include(${CMAKE_CURRENT_LIST_DIR}/pipeline/pipeline.cmake)
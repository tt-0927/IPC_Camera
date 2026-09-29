# event.cmake
#
# AI 检测结果标准化与事件分发 Pipeline 模块入口
# 由消费方业务工程显式 include（如 hi3516_ipc/main_app/CMakeLists.txt）；
# 本模块依赖 C++17（<optional> 等），工具链不支持的型号不引入即可跳过编译

# 引入 pipeline 子模块（包含 core/geometry/output/process 等）
include(${CMAKE_CURRENT_LIST_DIR}/pipeline/pipeline.cmake)
# event.cmake
#
# AI 检测结果标准化与事件分发 Pipeline 模块入口
# 由消费方业务工程显式 include（如 hi3516_ipc/main_app/CMakeLists.txt）。
# 本模块按 C++11 基线编写（标准由 ipc_share.cmake 固定），不依赖 C++17 库特性；
# 型号是否启用由画像宏 CAP_UNIFIED_EVENT_PIPELINE 控制，为 0 时全部文件编译为空。

# 引入 pipeline 子模块（包含 core/geometry/output/process 等）
include(${CMAKE_CURRENT_LIST_DIR}/pipeline/pipeline.cmake)

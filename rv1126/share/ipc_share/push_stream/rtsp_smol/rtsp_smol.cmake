# -----------------------------------------------------------------------------
# rtsp_smol.cmake：smolrtsp 薄封装接入片段
#
# 由 ipc_share/push_stream/push_stream.cmake 在 IPC_RTSP_BACKEND=smolrtsp 时 include。
# 产品构建的职责只有两项：
#   1. 收集本目录的 CRtspServer 薄封装源码；
#   2. 使用 ipc_platform/lib/rtsp_smol/lib/ 已由平台导入的 RTSP target。
#
# cam_share/media/ipc_rtspserver/ 本体由平台库 CMake 预编译，不能在这里再次 add_subdirectory。
# -----------------------------------------------------------------------------

# 薄封装使用平台归档随附的公共头文件；库 target 本身由 ipc.cmake 提前导入。
if(NOT DEFINED RTSP_SMOL_INCLUDE_DIR OR
   NOT EXISTS "${RTSP_SMOL_INCLUDE_DIR}/ipc_rtsp/server.h")
    message(FATAL_ERROR
        "未找到平台 smolrtsp 公共头文件，请检查 ipc_platform/lib/rtsp_smol/include/")
endif()
if(NOT TARGET ipc_rtspserver)
    message(FATAL_ERROR
        "平台未提供 ipc_rtspserver target，请检查 ipc_platform/lib/rtsp_smol/")
endif()

# 本目录源文件按既有 aux_source_directory 约定加入全局 SRC_LIST。
aux_source_directory("${CMAKE_CURRENT_LIST_DIR}" RTSP_SMOL_LIST)
list(APPEND SRC_LIST ${RTSP_SMOL_LIST})
include_directories("${CMAKE_CURRENT_LIST_DIR}")
include_directories("${RTSP_SMOL_INCLUDE_DIR}")

# 静态库 target 与 IPC_RTSP_LINK_LIBS 均由平台 CMake 提前提供；本文件不创建库。
message(STATUS "RTSP 后端: smolrtsp（平台归档 ${RTSP_SMOL_ARCHIVE}）")

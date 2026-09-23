# rtsp_smol.cmake
#
# 产品工程直接导入预编译的静态归档，目录布局与其他平台库一致：
#   ipc_platform/lib/rtsp_smol/lib/libipc_rtspserver.a
#
# 归档由 cam_opensource_library/libIpcRtspServer/compile.sh --sync-ipc-platform 发布；
# stream 构建阶段只负责导入和链接，不在 share/ipc_share/push_stream/rtsp_smol/ 中
# 再次 add_subdirectory。

set(RTSP_SMOL_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(RTSP_SMOL_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(RTSP_SMOL_ARCHIVE "${RTSP_SMOL_LIBRARY_DIR}/libipc_rtspserver.a")
set(RTSP_SMOL_PROTOCOL_ARCHIVE "${RTSP_SMOL_LIBRARY_DIR}/libsmolrtsp.a")

if(NOT EXISTS "${RTSP_SMOL_ARCHIVE}" OR
   NOT EXISTS "${RTSP_SMOL_PROTOCOL_ARCHIVE}")
    message(FATAL_ERROR
        "缺少 smolrtsp RTSP 静态库：${RTSP_SMOL_LIBRARY_DIR}\n"
        "需要 libipc_rtspserver.a 和 libsmolrtsp.a；请先在\n"
        "cam_opensource_library/libIpcRtspServer 执行\n"
        "./compile.sh <cross-prefix> --sync-ipc-platform <本目录>。")
endif()
if(NOT EXISTS "${RTSP_SMOL_INCLUDE_DIR}/ipc_rtsp/server.h")
    message(FATAL_ERROR
        "缺少 ipc_rtspserver 公共头文件：${RTSP_SMOL_INCLUDE_DIR}")
endif()

if(NOT TARGET ipc_rtsp_protocol)
    add_library(ipc_rtsp_protocol STATIC IMPORTED GLOBAL)
    set_target_properties(ipc_rtsp_protocol PROPERTIES
        IMPORTED_LOCATION "${RTSP_SMOL_PROTOCOL_ARCHIVE}")
endif()

if(NOT TARGET ipc_rtspserver)
    add_library(ipc_rtspserver STATIC IMPORTED GLOBAL)
    set_target_properties(ipc_rtspserver PROPERTIES
        IMPORTED_LOCATION "${RTSP_SMOL_ARCHIVE}"
        INTERFACE_INCLUDE_DIRECTORIES "${RTSP_SMOL_INCLUDE_DIR}")
    target_link_libraries(ipc_rtspserver INTERFACE ipc_event_core ipc_rtsp_protocol)
endif()

set(IPC_RTSP_LINK_LIBS ipc_rtspserver CACHE INTERNAL
    "平台提供的 smolrtsp RTSP 静态库")

message(STATUS "平台 smolrtsp：${RTSP_SMOL_ARCHIVE}")

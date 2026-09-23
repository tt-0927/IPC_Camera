# -----------------------------------------------------------------------------
# push_stream.cmake：推流模块（RTSP / RTMP）
#
# RTSP 后端通过 IPC_RTSP_BACKEND 二选一，两个后端提供同名同签名的
# CRtspServer（头文件都叫 rtsp_server.h），因此业务调用方无需改动：
#   live555  —— 旧实现（ipc_share/push_stream/rtsp/，显式回滚用）
#   smolrtsp —— 新实现（ipc_share/push_stream/rtsp_smol/ + 独立库 ipc_rtspserver，默认）
# 两个目录绝不会同时进入 include 路径，避免头文件冲突。
# -----------------------------------------------------------------------------
include( ${CMAKE_CURRENT_LIST_DIR}/common/common.cmake )

if(NOT DEFINED IPC_RTSP_BACKEND)
    set(IPC_RTSP_BACKEND "live555" CACHE STRING "RTSP 后端：smolrtsp（新）| live555（旧回滚）")
endif()

if(IPC_RTSP_BACKEND STREQUAL "smolrtsp")
    include( ${CMAKE_CURRENT_LIST_DIR}/rtsp_smol/rtsp_smol.cmake )
elseif(IPC_RTSP_BACKEND STREQUAL "live555")
    include( ${CMAKE_CURRENT_LIST_DIR}/rtsp/rtsp.cmake )
else()
    message(FATAL_ERROR
        "IPC_RTSP_BACKEND 必须是 smolrtsp 或 live555，当前为：${IPC_RTSP_BACKEND}")
endif()

if(IPC_CAP_RTMP_PUSH)
    include( ${CMAKE_CURRENT_LIST_DIR}/rtmp/rtmp.cmake )
endif()

#源文件
set (PUSH_STREAM_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${PUSH_STREAM_PATH})
    aux_source_directory (${item} PUSH_STREAM_LIST)
endforeach()
#头文件
set (PUSH_STREAM_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${PUSH_STREAM_INCLUDE})
    include_directories ( ${item} ) 
endforeach()
list(APPEND SRC_LIST ${PUSH_STREAM_LIST})

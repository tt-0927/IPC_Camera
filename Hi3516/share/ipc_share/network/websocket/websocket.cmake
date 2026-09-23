# WebSocket 后端二选一（对齐 push_stream 的 IPC_RTSP_BACKEND 机制）：
#   evws           libevent evws 后端（目标后端；本文件自行引入平台 libevent.cmake，
#                  与 IPC_RTSP_BACKEND 取值解耦；链接侧需 ipc_event_core /
#                  ipc_event_pthreads / ipc_event_openssl）
#   libwebsockets  lws 4.3.0 动态库后端（现状，回退保留）
# 两个后端提供同名同签名的 WebSocketServer 封装（Net::WebSocketServer 不感知差异）。
if(NOT DEFINED IPC_WS_BACKEND)
    set(IPC_WS_BACKEND "libwebsockets" CACHE STRING "WS 后端：evws（新）| libwebsockets（旧回滚）")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/../../encrypt/md5/md5.cmake)

if(IPC_WS_BACKEND STREQUAL "evws")
    add_compile_definitions(IPC_WS_BACKEND_EVWS)

    # 平台 libevent 由本文件自行引入（幂等：libevent.cmake 内有 target 存在性
    # 保护），live555 RTSP 设备也可单独使用 evws WS 后端。
    if(NOT DEFINED IPC_PLATFORM_PATH)
        message(FATAL_ERROR "IPC_WS_BACKEND=evws 需要 IPC_PLATFORM_PATH 变量（由 hi3516_ipc/ipc.cmake 定义）")
    endif()
    include(${IPC_PLATFORM_PATH}/lib/libevent/libevent.cmake)
    if(NOT TARGET ipc_event_core)
        message(FATAL_ERROR
            "IPC_WS_BACKEND=evws 引入平台 libevent 失败，请检查 "
            "${IPC_PLATFORM_PATH}/lib/libevent/ 目录与归档完整性")
    endif()
    get_target_property(WS_EVENT2_INCLUDE_DIR ipc_event_core INTERFACE_INCLUDE_DIRECTORIES)
    if(NOT WS_EVENT2_INCLUDE_DIR)
        message(FATAL_ERROR "ipc_event_core 未提供 include 路径，请检查平台 libevent.cmake")
    endif()
    include_directories(${WS_EVENT2_INCLUDE_DIR})
    # TLS 支持（EvwsServer 的 __has_include(<openssl/ssl.h>) 探测）
    if(DEFINED OPENSSL_INCLUDE_DIR)
        include_directories(${OPENSSL_INCLUDE_DIR})
    endif()

    set(WS_SRC_PATH
        ${CMAKE_CURRENT_LIST_DIR}/
        ${CMAKE_CURRENT_LIST_DIR}/libevent
        ${CMAKE_CURRENT_LIST_DIR}/../
    )
elseif(IPC_WS_BACKEND STREQUAL "libwebsockets")
    set(WS_SRC_PATH
        ${CMAKE_CURRENT_LIST_DIR}/
        ${CMAKE_CURRENT_LIST_DIR}/libwebsockets
        ${CMAKE_CURRENT_LIST_DIR}/../
        ${CMAKE_CURRENT_LIST_DIR}/../../shared_library/websocket
        ${CMAKE_CURRENT_LIST_DIR}/../../shared_library/websocket/libwebsockets
        ${CMAKE_CURRENT_LIST_DIR}/../../shared_library/openssl
    )
else()
    message(FATAL_ERROR
        "IPC_WS_BACKEND 必须是 evws 或 libwebsockets，当前为：${IPC_WS_BACKEND}")
endif()

foreach(item ${WS_SRC_PATH})
    include_directories ( ${item} )
    aux_source_directory (${item} WS_SRC_LIST)
endforeach()

list(APPEND SRC_LIST ${WS_SRC_LIST} )

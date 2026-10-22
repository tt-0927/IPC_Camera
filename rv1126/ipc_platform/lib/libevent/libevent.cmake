# libevent.cmake
#
# 产品工程直接复用平台目录中的 OpenIPC libevent 静态归档，目录布局与其他平台库一致：
#   ipc_platform/lib/libevent/include/event2/*.h
#   ipc_platform/lib/libevent/lib/libevent.a
#
# libevent.a 必须由 cam_opensource_library/libevent-openipc 使用目标工具链构建
# （产物在其 build/ 下），Hi3516 归档已包含 BROKEN_MMAP；这里不在业务工程内
# 重新编译第三方源码。

set(LIBEVENT_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(LIBEVENT_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(LIBEVENT_ARCHIVE "${LIBEVENT_LIBRARY_DIR}/libevent.a")
set(LIBEVENT_CONFIG_HEADER "${LIBEVENT_INCLUDE_DIR}/event2/event-config.h")

if(NOT EXISTS "${LIBEVENT_ARCHIVE}")
    message(FATAL_ERROR
        "缺少平台 libevent 静态库：${LIBEVENT_ARCHIVE}\n"
        "请先在 cam_opensource_library/libevent-openipc 执行 ./compile.sh <cross-prefix>，\n"
        "并将其 build/ 下的 libevent.a 与 include/ 同步到：${CMAKE_CURRENT_LIST_DIR}")
endif()
if(NOT EXISTS "${LIBEVENT_CONFIG_HEADER}")
    message(FATAL_ERROR
        "缺少平台 libevent 配置头：${LIBEVENT_CONFIG_HEADER}\n"
        "请同步 libevent-openipc build/include/ 目录。")
endif()

if(NOT TARGET ipc_event_core)
    add_library(ipc_event_core STATIC IMPORTED GLOBAL)
    set_target_properties(ipc_event_core PROPERTIES
        IMPORTED_LOCATION "${LIBEVENT_ARCHIVE}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBEVENT_INCLUDE_DIR}")
endif()

# Web 控制通道（evws 后端）依赖的补充归档：libevent.a 仅含 core+extra 源码，
# evthread_use_pthreads 与 bufferevent_openssl_* 分别在独立归档中。
# 两者与 libevent.a 同源同工具链（libevent-openipc compile.sh 产物）。
foreach(_evcomp pthreads openssl)
    if(NOT TARGET ipc_event_${_evcomp})
        set(_evcomp_archive "${LIBEVENT_LIBRARY_DIR}/libevent_${_evcomp}.a")
        if(EXISTS "${_evcomp_archive}")
            add_library(ipc_event_${_evcomp} STATIC IMPORTED GLOBAL)
            set_target_properties(ipc_event_${_evcomp} PROPERTIES
                IMPORTED_LOCATION "${_evcomp_archive}")
        else()
            message(WARNING "缺少 libevent_${_evcomp}.a（evws 后端需要），请重新同步 libevent-openipc 产物")
        endif()
    endif()
    unset(_evcomp_archive)
endforeach()

set(IPC_RTSP_LIBEVENT_CORE_TARGET ipc_event_core CACHE INTERNAL
    "平台提供的 libevent core 静态 target")

message(STATUS "平台 libevent：${LIBEVENT_ARCHIVE}")

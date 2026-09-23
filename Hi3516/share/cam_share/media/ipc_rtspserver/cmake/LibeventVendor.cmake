# -----------------------------------------------------------------------------
# libevent 离线构建/SDK 接线
#
# 上游：OpenIPC/libevent（私有 fork，2.2 系），
# pin `694decef35717d8955aa34ba4d2baaaf61c9e4a9`（"ws: do not emit a data frame
# after the close frame"）。
#
# `SOURCE` 模式从源码树 add_subdirectory，适合主机验证、CI 和个人开发；
# `SDK` 模式从第三方仓库产出的 include/ + lib/ 导入静态库，适合产品构建。
# 两种模式都通过统一的 `ipc_event_*` target 暴露，避免业务工程手工拼链接参数。
# -----------------------------------------------------------------------------

set(IPC_RTSP_LIBEVENT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../third_party/libevent"
    CACHE PATH "SOURCE 模式的 libevent 根目录（OpenIPC fork，pin 694decef）")
set(IPC_RTSP_LIBEVENT_MODE "SOURCE"
    CACHE STRING "libevent 接入模式：SOURCE 或 SDK")
set_property(CACHE IPC_RTSP_LIBEVENT_MODE PROPERTY STRINGS SOURCE SDK)
set(IPC_RTSP_LIBEVENT_SDK_DIR ""
    CACHE PATH "SDK 模式的 libevent staging 根目录（包含 include/ 和 lib/）")
option(IPC_RTSP_LIBEVENT_BROKEN_MMAP
       "在 libevent 编译目标上定义 BROKEN_MMAP（目标平台需明确验证）" OFF)

if(NOT IPC_RTSP_LIBEVENT_MODE STREQUAL "SOURCE" AND
   NOT IPC_RTSP_LIBEVENT_MODE STREQUAL "SDK")
    message(FATAL_ERROR
        "IPC_RTSP_LIBEVENT_MODE 必须是 SOURCE 或 SDK，当前为："
        "${IPC_RTSP_LIBEVENT_MODE}")
endif()

# 父工程的平台层已经按 ipc_platform/lib/* 规范提供静态归档时，直接复用该 target。
# 这样产品构建不会再次编译 third_party/libevent，也不会要求 build.sh 传额外参数。
if(TARGET ipc_event_core)
    message(STATUS "复用平台提供的 ipc_event_core 静态 target")
    set(IPC_RTSP_LIBEVENT_CORE_TARGET ipc_event_core CACHE INTERNAL
        "统一 libevent core CMake target")
    return()
endif()

# -----------------------------------------------------------------------------
# SDK 模式：导入第三方仓库输出的静态组件。
# -----------------------------------------------------------------------------
function(_ipc_rtsp_import_event_archive component)
    set(_archive "${IPC_RTSP_LIBEVENT_SDK_DIR}/lib/libevent_${component}.a")
    set(_archive_target "ipc_event_${component}_archive")
    set(_wrapper_target "ipc_event_${component}")

    if(NOT EXISTS "${_archive}")
        message(FATAL_ERROR
            "SDK 模式缺少 libevent 组件：${_archive}\n"
            "请先在 cam_opensource_library/libevent-openipc 中构建目标 SDK。")
    endif()

    if(NOT TARGET ${_archive_target})
        add_library(${_archive_target} STATIC IMPORTED GLOBAL)
        set_target_properties(${_archive_target} PROPERTIES
            IMPORTED_LOCATION "${_archive}"
            INTERFACE_INCLUDE_DIRECTORIES "${IPC_RTSP_LIBEVENT_SDK_DIR}/include")
    endif()

    if(NOT TARGET ${_wrapper_target})
        add_library(${_wrapper_target} INTERFACE)
        target_link_libraries(${_wrapper_target} INTERFACE ${_archive_target})
    endif()
endfunction()

if(IPC_RTSP_LIBEVENT_MODE STREQUAL "SDK")
    # 上层 platform CMake 可能已经导入了统一的 ipc_event_core；复用它，避免同一
    # 进程中因 RTSP 子项目再次导入而形成两套 archive target。
    if(TARGET ipc_event_core)
        message(STATUS "复用上层已提供的 ipc_event_core target")
        set(IPC_RTSP_LIBEVENT_CORE_TARGET ipc_event_core CACHE INTERNAL
            "统一 libevent core CMake target")
        return()
    endif()

    if(NOT IPC_RTSP_LIBEVENT_SDK_DIR)
        message(FATAL_ERROR
            "SDK 模式必须设置 -DIPC_RTSP_LIBEVENT_SDK_DIR=<staging 根目录>。")
    endif()
    if(NOT EXISTS "${IPC_RTSP_LIBEVENT_SDK_DIR}/include/event2/event-config.h")
        message(FATAL_ERROR
            "SDK 模式缺少目标配置头文件："
            "${IPC_RTSP_LIBEVENT_SDK_DIR}/include/event2/event-config.h")
    endif()

    _ipc_rtsp_import_event_archive(core)

    # 后续 Web 阶段使用。第一阶段只要求 core；extra/opssl 不存在时不报错，
    # 避免 RTSP-only SDK 被迫携带 HTTP/WebSocket/TLS 组件。
    if(EXISTS "${IPC_RTSP_LIBEVENT_SDK_DIR}/lib/libevent_extra.a")
        _ipc_rtsp_import_event_archive(extra)
        target_link_libraries(ipc_event_extra_archive INTERFACE ipc_event_core)
    endif()
    if(EXISTS "${IPC_RTSP_LIBEVENT_SDK_DIR}/lib/libevent_openssl.a")
        _ipc_rtsp_import_event_archive(openssl)
        target_link_libraries(ipc_event_openssl_archive INTERFACE ipc_event_core)
    endif()
    if(EXISTS "${IPC_RTSP_LIBEVENT_SDK_DIR}/lib/libevent_pthreads.a")
        _ipc_rtsp_import_event_archive(pthreads)
    endif()

    set(IPC_RTSP_LIBEVENT_CORE_TARGET ipc_event_core CACHE INTERNAL
        "统一 libevent core CMake target")
    return()
endif()

# -----------------------------------------------------------------------------
# SOURCE 模式：上游构建选项全部显式固定，避免受父工程全局 cache 影响。
# -----------------------------------------------------------------------------
if(NOT EXISTS "${IPC_RTSP_LIBEVENT_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR
        "未找到 SOURCE 模式的 libevent：${IPC_RTSP_LIBEVENT_ROOT}/CMakeLists.txt\n"
        "请用 -DIPC_RTSP_LIBEVENT_ROOT=<独立源码工作树> 指定，或切换为 SDK 模式。")
endif()

# CMake 4.x 会拒绝 `cmake_minimum_required` 低于 3.5 的子项目；libevent 历史版本
# 声明过 3.1，这里统一兜底（对本 fork 无害）。
if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

set(EVENT__LIBRARY_TYPE STATIC CACHE STRING "libevent 库类型" FORCE)
set(EVENT__DISABLE_SAMPLES ON CACHE BOOL "不构建 sample" FORCE)
set(EVENT__DISABLE_TESTS ON CACHE BOOL "不构建 test" FORCE)
set(EVENT__DISABLE_BENCHMARK ON CACHE BOOL "不构建 benchmark" FORCE)
set(EVENT__DISABLE_REGRESS ON CACHE BOOL "不构建 regress" FORCE)
set(EVENT__DISABLE_RPC ON CACHE BOOL "不构建 evrpc" FORCE)
set(EVENT__DISABLE_EVENT_TAGGING ON CACHE BOOL "不构建 evtag（依赖 RPC 已关）" FORCE)
set(EVENT__DISABLE_DEBUG_MODE ON CACHE BOOL "关闭调试断言/verbose" FORCE)
set(EVENT__DISABLE_MM_REPLACEMENT ON CACHE BOOL "不接管 malloc/free" FORCE)
# RTSP 本身不需要 TLS；Web 控制通道迁移时改为 OFF 并链接 event_openssl。
set(EVENT__DISABLE_OPENSSL ON CACHE STRING "不使用 OpenSSL（迁移 Web 通道时改 OFF）" FORCE)
set(EVENT__DISABLE_MBEDTLS ON CACHE STRING "不使用 mbedTLS" FORCE)
# evws 保留：Web 控制通道迁移要用；线程 API 保留：不等于直接跨线程调用。
set(EVENT__DISABLE_WS OFF CACHE BOOL "保留 evws" FORCE)
set(EVENT__DISABLE_THREAD_SUPPORT OFF CACHE BOOL "保留线程 API" FORCE)

add_subdirectory("${IPC_RTSP_LIBEVENT_ROOT}"
                 "${CMAKE_CURRENT_BINARY_DIR}/libevent"
                 EXCLUDE_FROM_ALL)

if(NOT TARGET event_core)
    message(FATAL_ERROR "SOURCE 模式的 libevent 没有生成 event_core target。")
endif()

# 让 libevent 的函数/数据各自成节，最终链接才能按符号裁剪。
# 注意：`event_core` 是 INTERFACE 目标，真正编译的是 `*_static`。
foreach(_ipc_rtsp_lev_target
        event_core_static event_extra_static event_openssl_static
        event_pthreads_static event_core_shared event_extra_shared
        event_openssl_shared event_pthreads_shared)
    if(TARGET ${_ipc_rtsp_lev_target})
        target_compile_options(${_ipc_rtsp_lev_target} PRIVATE
            -ffunction-sections -fdata-sections)
        if(IPC_RTSP_LIBEVENT_BROKEN_MMAP)
            target_compile_definitions(${_ipc_rtsp_lev_target} PRIVATE BROKEN_MMAP)
        endif()
    endif()
endforeach()

# 用项目自己的稳定名字包一层上游 target。后续 Web/HTTP 可以复用同一套 target，
# 但产品代码不需要依赖 `event_core` 这个上游 target 名称。
function(_ipc_rtsp_wrap_event_target component upstream_target)
    set(_wrapper_target "ipc_event_${component}")
    if(NOT TARGET ${_wrapper_target})
        add_library(${_wrapper_target} INTERFACE)
        target_link_libraries(${_wrapper_target} INTERFACE ${upstream_target})
    endif()
endfunction()

_ipc_rtsp_wrap_event_target(core event_core)
if(TARGET event_extra)
    _ipc_rtsp_wrap_event_target(extra event_extra)
endif()
if(TARGET event_openssl)
    _ipc_rtsp_wrap_event_target(openssl event_openssl)
endif()
if(TARGET event_pthreads)
    _ipc_rtsp_wrap_event_target(pthreads event_pthreads)
endif()

set(IPC_RTSP_LIBEVENT_CORE_TARGET ipc_event_core CACHE INTERNAL
    "统一 libevent core CMake target")

# -----------------------------------------------------------------------------
# ipc_rtspserver 构建选项
#
# 所有选项都可在父工程里通过 -D 或 set(... CACHE ...) 覆盖。
# -----------------------------------------------------------------------------

# vendored 第三方源码根目录（smolrtsp 家族）。
set(IPC_RTSP_VENDOR_ROOT "${CMAKE_CURRENT_LIST_DIR}/../third_party/smolrtsp"
    CACHE PATH "vendored smolrtsp 家族根目录")

option(IPC_RTSP_SHARED "构建共享库而非静态库" OFF)
set(IPC_RTSP_LIBEVENT_MODE "SOURCE" CACHE STRING "libevent 接入模式：SOURCE 或 SDK")
set_property(CACHE IPC_RTSP_LIBEVENT_MODE PROPERTY STRINGS SOURCE SDK)
set(IPC_RTSP_LIBEVENT_SDK_DIR "" CACHE PATH "SDK 模式的 libevent staging 根目录")
option(IPC_RTSP_LIBEVENT_BROKEN_MMAP
       "在 libevent 编译目标上定义 BROKEN_MMAP（目标平台需明确验证）" OFF)
option(IPC_RTSP_EXCEPTIONS "库内启用 exception/RTTI（默认关闭）" OFF)
option(IPC_RTSP_WITH_MJPEG "启用 smolrtsp MJPEG payload（RFC 2435）" ON)
option(IPC_RTSP_BUILD_TESTS "构建主机侧单元测试" ON)
option(IPC_RTSP_BUILD_POC "构建主机侧验证程序（文件媒体源）" ON)
option(IPC_RTSP_WARNINGS_AS_ERRORS "把编译告警当错误" OFF)

if(NOT IPC_RTSP_BUILD_POC)
    set(IPC_RTSP_BUILD_TESTS OFF CACHE BOOL "构建主机侧单元测试" FORCE)
endif()

find_package(Threads REQUIRED)

if(IPC_RTSP_VENDOR_ROOT AND NOT EXISTS "${IPC_RTSP_VENDOR_ROOT}/smolrtsp/CMakeLists.txt")
    message(FATAL_ERROR
        "未找到 vendored smolrtsp：${IPC_RTSP_VENDOR_ROOT}/smolrtsp/CMakeLists.txt\n"
        "请用 -DIPC_RTSP_VENDOR_ROOT=<third_party/smolrtsp 路径> 指定。")
endif()

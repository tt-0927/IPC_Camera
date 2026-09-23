# -----------------------------------------------------------------------------
# ipc_rtspserver：面向 IPC 的轻量 RTSP Server 库
#
# 独立可构建（主机 PoC / 单元测试），也可被父工程 add_subdirectory 复用。
# 协议核心为 vendored 的 OpenIPC/smolrtsp（C99），本库为 C++17。
# 详细设计见 docs/smolrtsp_rtspserver_docs/19~23。
# -----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.16)

project(ipc_rtspserver LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(IpcRtspServerOptions)
include(SmolrtspOffline)
include(LibeventVendor)

# -----------------------------------------------------------------------------
# 产物身份：用于板端日志确认源码、静态归档与 stream 是否同代
# -----------------------------------------------------------------------------
# 发布/SDK 构建可显式传入这两个值；本地 Git 工作区默认自动解析 revision，
# 并在本库目录存在未提交改动时追加 -dirty。源码包没有 .git 时明确记为 unknown。
set(IPC_RTSP_REVISION_OVERRIDE "" CACHE STRING "覆盖 ipc_rtspserver 源码 revision")
set(IPC_RTSP_BUILD_ID "" CACHE STRING "发布系统分配的 ipc_rtspserver 产物标识")

set(IPC_RTSP_SOURCE_REVISION "unknown")
if(IPC_RTSP_REVISION_OVERRIDE)
    set(IPC_RTSP_SOURCE_REVISION "${IPC_RTSP_REVISION_OVERRIDE}")
else()
    find_package(Git QUIET)
    if(GIT_FOUND)
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE IPC_RTSP_GIT_REVISION_RESULT
            OUTPUT_VARIABLE IPC_RTSP_GIT_REVISION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
        if(IPC_RTSP_GIT_REVISION_RESULT EQUAL 0 AND IPC_RTSP_GIT_REVISION)
            set(IPC_RTSP_SOURCE_REVISION "${IPC_RTSP_GIT_REVISION}")
            execute_process(
                COMMAND "${GIT_EXECUTABLE}" status --porcelain --untracked-files=normal -- .
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                RESULT_VARIABLE IPC_RTSP_GIT_STATUS_RESULT
                OUTPUT_VARIABLE IPC_RTSP_GIT_STATUS
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET)
            if(IPC_RTSP_GIT_STATUS_RESULT EQUAL 0 AND IPC_RTSP_GIT_STATUS)
                string(APPEND IPC_RTSP_SOURCE_REVISION "-dirty")
            endif()
        endif()
    endif()
endif()

if(IPC_RTSP_BUILD_ID)
    set(IPC_RTSP_EFFECTIVE_BUILD_ID "${IPC_RTSP_BUILD_ID}")
else()
    set(IPC_RTSP_EFFECTIVE_BUILD_ID "${IPC_RTSP_SOURCE_REVISION}")
endif()

# 与 third_party/README.md 的锁定基线保持一致；升级第三方时必须同步修改。
set(IPC_RTSP_SMOLRTSP_REVISION "8ae3b5115f7a77c383080b51cb7fdba1046f862f")
set(IPC_RTSP_SMOLRTSP_LIBEVENT_REVISION "b0329b4f8e72ff03f2b674ffdbfdc8553523ddf2")
set(IPC_RTSP_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${IPC_RTSP_GENERATED_DIR}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ipc_rtsp_build_info.h.in"
    "${IPC_RTSP_GENERATED_DIR}/ipc_rtsp_build_info.h"
    @ONLY)

# -----------------------------------------------------------------------------
# 协议核心：smolrtsp（vendored，禁止修改源码）
# -----------------------------------------------------------------------------
# 本库只用 H.264/H.265；MJPEG 由 IPC_RTSP_WITH_MJPEG 打开。
set(SMOLRTSP_WITH_H264   ON  CACHE BOOL "smolrtsp H.264" FORCE)
set(SMOLRTSP_WITH_H265   ON  CACHE BOOL "smolrtsp H.265" FORCE)
set(SMOLRTSP_WITH_H266   OFF CACHE BOOL "smolrtsp H.266" FORCE)
set(SMOLRTSP_WITH_AV1    OFF CACHE BOOL "smolrtsp AV1"   FORCE)
set(SMOLRTSP_WITH_JPEG   ${IPC_RTSP_WITH_MJPEG} CACHE BOOL "smolrtsp MJPEG" FORCE)
set(SMOLRTSP_WITH_JPEGXS OFF CACHE BOOL "smolrtsp JPEG XS" FORCE)

add_subdirectory("${IPC_RTSP_VENDOR_ROOT}/smolrtsp" "${CMAKE_CURRENT_BINARY_DIR}/smolrtsp" EXCLUDE_FROM_ALL)

# -----------------------------------------------------------------------------
# 库目标
# -----------------------------------------------------------------------------
set(IPC_RTSP_SOURCES
    src/server.cpp
    # 隔离层：全项目唯一包含 smolrtsp 头文件的编译单元（C99）
    src/adapter/smolrtsp_shim.c
    src/support/log.cpp
    src/support/md5.cpp
    src/support/text.cpp
    src/support/time.cpp
    src/net/event_loop.cpp
    src/net/tcp_listener.cpp
    src/net/connection.cpp
    src/net/output_buffer.cpp
    src/net/udp_socket.cpp
    src/rtsp/url_router.cpp
    src/rtsp/session.cpp
    src/rtsp/sdp.cpp
    src/rtsp/controller.cpp
    src/auth/digest.cpp
    src/media/annexb_scanner.cpp
    src/media/codec_config.cpp
    src/media/frame_hub.cpp
    src/media/gop_cache.cpp
    src/media/frame_reader.cpp
    src/media/file_source.cpp
    src/media/jpeg_file_source.cpp
    src/media/jpeg_parser.cpp
    src/rtp/audio_payload.cpp
    src/rtp/rtp_sender.cpp
    src/rtp/rtcp_sender.cpp)

if(IPC_RTSP_SHARED)
    add_library(ipc_rtspserver SHARED ${IPC_RTSP_SOURCES})
    target_compile_definitions(ipc_rtspserver PRIVATE IPC_RTSP_BUILD_SHARED)
else()
    add_library(ipc_rtspserver STATIC ${IPC_RTSP_SOURCES})
endif()

target_include_directories(ipc_rtspserver
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${IPC_RTSP_GENERATED_DIR}")

target_compile_features(ipc_rtspserver PRIVATE cxx_std_17)
set_target_properties(ipc_rtspserver PROPERTIES POSITION_INDEPENDENT_CODE ON)

# libevent 用 PUBLIC：`net/event_loop.h` 会包含 `<event2/util.h>`，静态库的链接依赖
# 必须传递给最终目标（产品侧只包含 `ipc_rtsp/*.h`，不受影响）。`ipc_event_core`
# 同时覆盖 SOURCE/SDK 两种接入模式。
target_link_libraries(ipc_rtspserver PUBLIC ipc_event_core PRIVATE smolrtsp PRIVATE Threads::Threads)

if(IPC_RTSP_EXCEPTIONS)
    target_compile_options(ipc_rtspserver PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fexceptions;-frtti>)
else()
    # 嵌入式默认：C++ 侧不抛异常、不依赖 RTTI；全部错误走 Result。
    target_compile_options(ipc_rtspserver PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions;-fno-rtti>)
endif()

target_compile_options(ipc_rtspserver PRIVATE -Wall -Wextra -Wno-unused-parameter)

# 嵌入式体积前提：本库与 vendored libevent 的函数/数据各自成节，最终链接带
# `--gc-sections` 才能按符号裁剪。文档（22_ADR 的 D2-R1）中的体积数字都以此为前提。
target_compile_options(ipc_rtspserver PRIVATE -ffunction-sections -fdata-sections)
target_link_options(ipc_rtspserver INTERFACE -Wl,--gc-sections)

if(IPC_RTSP_WARNINGS_AS_ERRORS)
    target_compile_options(ipc_rtspserver PRIVATE -Werror)
endif()

# -----------------------------------------------------------------------------
# 主机验证程序与测试（父工程集成时可关闭）
# -----------------------------------------------------------------------------
if(IPC_RTSP_BUILD_POC)
    add_subdirectory(poc)
endif()

if(IPC_RTSP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

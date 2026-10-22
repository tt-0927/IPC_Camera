# -----------------------------------------------------------------------------
# smolrtsp 离线构建接线
#
# 上游 smolrtsp/CMakeLists.txt、datatype99、interface99 都用 FetchContent 从公网
# 拉取 99 宏库。本项目要求离线可复现，因此在 add_subdirectory 之前把
# FETCHCONTENT_SOURCE_DIR_<NAME> 指向 vendored 目录，并关闭全部下载。
#
# 注意：这里只改 FetchContent 行为，不修改 third_party 内任何源码。
# -----------------------------------------------------------------------------

foreach(_ipc_rtsp_dep slice99 metalang99 datatype99 interface99)
    string(TOUPPER "${_ipc_rtsp_dep}" _ipc_rtsp_dep_upper)

    if(NOT EXISTS "${IPC_RTSP_VENDOR_ROOT}/${_ipc_rtsp_dep}/CMakeLists.txt")
        message(FATAL_ERROR
            "缺少 vendored 宏库 ${_ipc_rtsp_dep}："
            "${IPC_RTSP_VENDOR_ROOT}/${_ipc_rtsp_dep}/CMakeLists.txt")
    endif()

    set(FETCHCONTENT_SOURCE_DIR_${_ipc_rtsp_dep_upper}
        "${IPC_RTSP_VENDOR_ROOT}/${_ipc_rtsp_dep}" CACHE PATH "vendored ${_ipc_rtsp_dep}" FORCE)
endforeach()

set(FETCHCONTENT_FULLY_DISCONNECTED ON CACHE BOOL "禁止 FetchContent 联网下载" FORCE)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "禁止 FetchContent 更新步骤" FORCE)

# smolrtsp 使用 target_precompile_headers；部分老交叉工具链（GCC 10 + musl）
# 下 PCH 会带来额外风险，这里提供一个变量供父工程在需要时关闭：
#   -DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON

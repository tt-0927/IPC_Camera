# -----------------------------------------------------------------------------
# 主机验证程序：文件媒体源 RTSP Server
#
# 用途：在没有板端 VENC 的环境下验证协议栈、背压与多客户端行为。
# 用法见 ipc_rtspserver/README.md。
# -----------------------------------------------------------------------------
add_executable(rtspserver_poc main.cpp)
target_link_libraries(rtspserver_poc PRIVATE ipc_rtspserver)
# PoC 是主机侧验证程序，允许使用库内部的文件媒体源与时间工具做测试注入。
target_include_directories(rtspserver_poc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")
target_compile_features(rtspserver_poc PRIVATE cxx_std_11)
target_compile_options(rtspserver_poc PRIVATE -Wall -Wextra)

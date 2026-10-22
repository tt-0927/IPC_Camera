# -----------------------------------------------------------------------------
# 主机侧单元测试（不依赖 gtest，使用极简断言框架）
# -----------------------------------------------------------------------------
add_library(rtsp_test_support STATIC test_support.cpp)
target_include_directories(rtsp_test_support PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
# 白盒单元测试需要访问库内部头文件（不随安装/对外接口暴露）。
target_include_directories(rtsp_test_support PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../src")
target_link_libraries(rtsp_test_support PUBLIC ipc_rtspserver)
target_compile_features(rtsp_test_support PUBLIC cxx_std_11)

function(ipc_rtsp_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE rtsp_test_support)
    target_compile_features(${name} PRIVATE cxx_std_11)
    target_compile_options(${name} PRIVATE -Wall -Wextra)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

ipc_rtsp_add_test(test_annexb)
ipc_rtsp_add_test(test_api_contract)
ipc_rtsp_add_test(test_digest)
ipc_rtsp_add_test(test_event_loop)
ipc_rtsp_add_test(test_frame_hub)
ipc_rtsp_add_test(test_frame_reader)
ipc_rtsp_add_test(test_gop_cache)
ipc_rtsp_add_test(test_jpeg_parser)
ipc_rtsp_add_test(test_output_buffer)
ipc_rtsp_add_test(test_url_router)
ipc_rtsp_add_test(test_sdp)
ipc_rtsp_add_test(test_server_lifecycle)
ipc_rtsp_add_test(test_connection_backpressure)
ipc_rtsp_add_test(test_smolrtsp_shim)
ipc_rtsp_add_test(test_audio_payload)

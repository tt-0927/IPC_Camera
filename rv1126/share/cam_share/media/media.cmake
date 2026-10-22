# media.cmake

# 引用cmake
include(${CMAKE_CURRENT_LIST_DIR}/calculate/calculate.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/codec/codec.cmake)
# include(${CMAKE_CURRENT_LIST_DIR}/enc_dec_tool/enc_dec_tool.cmake)
# include(${CMAKE_CURRENT_LIST_DIR}/libmedia/libmedia.cmake)

# ipc_rtspserver 是独立构建的媒体库；产品侧由 ipc_platform/lib/rtsp_smol 导入归档，
# 不在 cam_share 的共享源码入口中再次 add_subdirectory。
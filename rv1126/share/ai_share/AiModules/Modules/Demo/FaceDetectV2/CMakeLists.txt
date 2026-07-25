# 版本限定
cmake_minimum_required(VERSION 3.12)

# 设置程序名称
project("FaceDetectV2_Demo")

include(${CMAKE_CURRENT_LIST_DIR}/../../../../../platform/platform.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../../../../platform/lib/ffmpeg-4.3/ffmpeg-4.3.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../../../../platform/lib/opencv/opencv.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../../../../platform/lib/rockit/rockit.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../../../../platform/lib/zlog/zlog.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../../../../rockit/rockit.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../FaceDetect/FaceDetect.cmake)

# 设置生成文件存放路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../output/bin)

# 添加其他源文件与头文件
set(SHARE_SRC_PATH
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/data_structure/que/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/data_structure/buffer/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/unclassified/os/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/standard/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/network/common
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/ipc/sem/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/log/dlog/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/log/nslog/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/system/time/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/common/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/common/error/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/media/frame_rate/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/media/ffmpeg/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/media/ffmpeg/ffmpeg/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/media/ffmpeg/demux/
)
set(SHARE_INCLUDE_PATH
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/data_structure/que/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/data_structure/buffer/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/unclassified/os/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/standard/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/network/common
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/ipc/sem/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/log/dlog/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/log/nslog/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/system/time/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/common/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/common/error/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/media/frame_rate/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/media/ffmpeg/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/media/ffmpeg/ffmpeg/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/media/ffmpeg/demux/
    ${CMAKE_CURRENT_LIST_DIR}/
)
# 添加共享库的头文件
foreach(item ${SHARE_INCLUDE_PATH})
    include_directories(${item})
endforeach()
# 添加共享库的源文件
foreach(item ${SHARE_SRC_PATH})
    aux_source_directory(${item} SRC_LIST)
endforeach()

# 编译
add_executable(${PROJECT_NAME} main.cpp ${SRC_LIST} )

# 链接库路径
target_link_libraries(${PROJECT_NAME}
    asound
    drm
    rockchip_mpp
    rockit
    avcodec        
    avdevice        
    avfilter        
    avformat        
    avutil       
    swscale         
    swresample
    rga
    xml2
    freetype
    x264
    fdk-aac
    libzlog
    z
    
    libffmpeg-4.3
    librockit
    libopencv
    FaceDetect_Module
)



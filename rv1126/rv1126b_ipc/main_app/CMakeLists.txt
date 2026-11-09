# 版本限定
cmake_minimum_required(VERSION 3.16.2)

# 程序名称
SET(PRO_NAME "stream")
project(${PRO_NAME})

# ipc_share
message(${IPC_SHARE_PATH})
if(IPC_CAP_NETWORK_GAT1400)
    SET(ENABLE_GAT1400 ON)
endif()
include(${IPC_SHARE_PATH}/ipc_share.cmake)
include(${IPC_SHARE_PATH}/protocols/tvsdk/tvsdk.cmake)

# cam_share
include(${CAM_SHARE_PATH}/include/include.cmake)
include(${CAM_SHARE_PATH}/data/frontEncode/frontEncode.cmake)
include(${CAM_SHARE_PATH}/data/osa_base/container/container.cmake)
include(${CAM_SHARE_PATH}/data/osa_base/ipc/ipc.cmake)
include(${CAM_SHARE_PATH}/data/osa_base/srt/srt.cmake)
include(${CAM_SHARE_PATH}/data/osa_base/networkEvent/networkEvent.cmake)
include(${CAM_SHARE_PATH}/device_system/get_time/get_time.cmake)
include(${CAM_SHARE_PATH}/device_system/system/system.cmake)
include(${CAM_SHARE_PATH}/common/common.cmake)
include(${CAM_SHARE_PATH}/common/log/nslog/nslog.cmake)
include(${CAM_SHARE_PATH}/file_processing/xml/xml.cmake)
include(${CAM_SHARE_PATH}/media/codec/audio/g711/codec_g711.cmake)
include(${CAM_SHARE_PATH}/media/calculate/calculate.cmake)
include(${CAM_SHARE_PATH}/utils/utils.cmake)
include(${CAM_SHARE_PATH}/communication/network/curl_http/curl_http.cmake)
include(${CAM_SHARE_PATH}/utils/curl/curl.cmake)

# include(${AFSDK_PATH}/sdk_share/sdk_share.cmake)
# include(${AFSDK_PATH}/sdk_server/sdk_server.cmake)


set(SKIP_LINK_LIBROCKIT ON)

# ---- 课堂分析必用：推理封装层 + 跟踪 + 人脸 + 移动侦测（单产品仓，全量编入）----
set(AI_MODULE_TARGETS
    ByteTrack_CPU
    YoloUltralytics
    Yolov5
    Yolov5Point
    YoloUltralyticsPoint
    Attribute
    MoveDetect_Module
    FaceDetect_Module
    FaceAttribute_Module
)

include(${AI_SHARE_PATH}/AiModules/Inference/CPU/Tracker/ByteTrack/ByteTrack.cmake)
include(${AI_SHARE_PATH}/AiModules/Inference/RK/TargetDetection/YoloUltralytics/YoloUltralytics.cmake)
include(${AI_SHARE_PATH}/AiModules/Inference/RK/TargetDetection/Yolov5/Yolov5.cmake)
include(${AI_SHARE_PATH}/AiModules/Inference/RK/KeyPoint/Yolov5Point/Yolov5Point.cmake)
include(${AI_SHARE_PATH}/AiModules/Inference/RK/KeyPoint/YoloUltralyticsPoint/YoloUltralyticsPoint.cmake)
include(${AI_SHARE_PATH}/AiModules/Inference/RK/Classification/Attribute/Attribute.cmake)
include(${AI_SHARE_PATH}/AiModules/Modules/MoveDetect/MoveDetect.cmake)
include(${AI_SHARE_PATH}/AiModules/Modules/FaceDetect/FaceDetect.cmake)
include(${AI_SHARE_PATH}/AiModules/Modules/FaceAttribute/FaceAttribute.cmake)

# rk_pipeline
message(${RK_PIPELINE_PATH})
include(${RK_PIPELINE_PATH}/rk_pipeline.cmake)

# # ai_app
include(${CMAKE_CURRENT_LIST_DIR}/ai_app/ai_app.cmake)
# 设置库路径
# link_directories(
#     ${IPC_PLATFORM_PATH}/lib/openssl/lib/
# )

# 源文件
set (MAIN_SRC_PATH
    
)

# 头文件
set (MAIN_INC_PATH
    #include
    ${SRC_ROOT_PATH}/include
    # ${IPC_PLATFORM_PATH}/lib/openssl/include/
 )

foreach(item ${MAIN_SRC_PATH})
    aux_source_directory (${item} MAIN_LIST)
endforeach()

foreach(item ${MAIN_INC_PATH})
    include_directories (${item})
endforeach()

# 过滤
set(GREP_DIR "demo|Demo|ai_app")
# set(GREP_DIR "demo|Demo")

# 获取本程序子目录
execute_process(
    COMMAND find ${CMAKE_CURRENT_LIST_DIR}/  -type d 
    COMMAND grep -vE ${GREP_DIR} # 过滤目录 U
    OUTPUT_VARIABLE dirs_list
)

message(${dirs_list})
string(REPLACE "\n" ";" dirs_list ${dirs_list})

# 添加所有源文件与头文件
foreach(item ${dirs_list})
    include_directories(${item})
    aux_source_directory(${item} STREAM_LIST)
endforeach()

add_executable (${PRO_NAME} ${MAIN_LIST} ${STREAM_LIST} ${SRC_LIST} ${SOURCE_PATH})

#连接库
#查看platform下的库文件cmake
target_link_libraries(${PROJECT_NAME} 
    # system
    libz
    libfreetype
    libiconv
    # third
    libxml2
    #libx264
    #libx265
    #libcrypto
    #libsrt
    #libfaac
    libfdk-aac
    libffmpeg
    libzlog
    libwebsockets
    libSDL2
    libSDL2_ttf
    libsqlite3
    libeXosip2
    libosip2
    libosipparser2
    libpugixml
    libopenssl
    libupnp
    libsnmp++
    libagent++
    libRtspServer
    # libtwolame
    libonvif
    # libgmssl
    NetTVSDKServer
    libmqtt
    #librtmp
    # rk_pipeline
    -lasound
    -lrga
    -lrockchip_mpp
    -ldrm
    #-lrknnrt
    -lrockit
    -lrkaiq
    -lsmartIr
    #-lrockiva
    libopencv
    #libopencvfont
    librknnrt
    # 算法模块目标：课堂分析必用的推理/跟踪/人脸/移动侦测模块（include 与 link 同源）
    ${AI_MODULE_TARGETS}
    libmqtt
    libcurl
)

# target_link_libraries(${PROJECT_NAME}
#     ssl
#     crypto
# )

message("==========> Generate " ${PRO_NAME} " Makefile Success <==========")

# 版本限定
cmake_minimum_required(VERSION 3.16.2)

# 程序名称
SET(PRO_NAME "stream")
project(${PRO_NAME})

# ipc_share
message(${IPC_SHARE_PATH})
if (DEVICE_TYPE STREQUAL "TV-3881T" OR DEVICE_TYPE STREQUAL "TV-3882TI")
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

include(${AI_SHARE_PATH}/AiModules/Modules/SleepOnDutyDetect/SleepOnDutyDetect.cmake)
include(${AI_SHARE_PATH}/AiModules/Modules/TripDetect/TripDetect.cmake)
include(${AI_SHARE_PATH}/AiModules/Inference/CPU/Tracker/ByteTrack/ByteTrack.cmake)
include(${AI_SHARE_PATH}/AiModules/Inference/RK/TargetDetection/YoloUltralytics/YoloUltralytics.cmake)
include(${AI_SHARE_PATH}/AiModules/Inference/RK/TargetDetection/Yolov5/Yolov5.cmake)
include(${AI_SHARE_PATH}/AiModules/Inference/RK/KeyPoint/Yolov5Point/Yolov5Point.cmake)
include(${AI_SHARE_PATH}/AiModules/Inference/RK/KeyPoint/YoloUltralyticsPoint/YoloUltralyticsPoint.cmake)
include(${AI_SHARE_PATH}/AiModules/Inference/RK/Classification/Attribute/Attribute.cmake)
include(${AI_SHARE_PATH}/AiModules/Modules/MoveDetect/MoveDetect.cmake)
include(${AI_SHARE_PATH}/AiModules/Modules/CameraObstruction/CameraObstruction.cmake)
include(${AI_SHARE_PATH}/AiModules/Inference/RK/Classification/ImageFeature/ImageFeature.cmake)
include(${AI_SHARE_PATH}/AiModules/Modules/FaceDetect/FaceDetect.cmake)
include(${AI_SHARE_PATH}/AiModules/Modules/FaceQualityAssessment/FaceQualityAssessment.cmake)
include(${AI_SHARE_PATH}/AiModules/Modules/FaceAttribute/FaceAttribute.cmake)
# include(${AI_SHARE_PATH}/AiModules/Modules/HelmetDetect/HelmetDetect.cmake)
# include(${AI_SHARE_PATH}/AiModules/Modules/ReflectiveDetect/ReflectiveDetect.cmake)
# include(${AI_SHARE_PATH}/AiModules/Modules/SmokingDetect/SmokingDetect.cmake)
# include(${AI_SHARE_PATH}/AiModules/Modules/FenceDetect/FenceDetect.cmake)
# include(${AI_SHARE_PATH}/AiModules/Modules/PMNMDetect/PMNMDetect.cmake)
# include(${AI_SHARE_PATH}/AiModules/Modules/ElectricScooterDetect/ElectricScooterDetect.cmake)
# include(${AI_SHARE_PATH}/AiModules/Modules/VehicleDetect/VehicleDetect.cmake)
# include(${AI_SHARE_PATH}/AiModules/Modules/PlayPhoneDetect/PlayPhoneDetect.cmake)
# include(${AI_SHARE_PATH}/AiModules/Modules/SafetyropeAndSoilDetect/SafetyropeAndSoilDetect.cmake)
# include(${AI_SHARE_PATH}/AiModules/Modules/HumanAreaDetect/HumanAreaDetect.cmake)
# include(${AI_SHARE_PATH}/AiModules/Modules/FireDetect/FireDetect.cmake)
# include(${AI_SHARE_PATH}/AiModules/Modules/StreeDetect/StreeDetect.cmake)
# include(${AI_SHARE_PATH}/AiModules/Modules/RubishDetect/RubishDetect.cmake)
# include(${AI_SHARE_PATH}/AiModules/Modules/ConstructionEncroachmentRoadDetect/ConstructionEncroachmentRoadDetect.cmake)
include(${AI_SHARE_PATH}/AiModules/Modules/LicensePlateCognition/LicensePlateCognition.cmake)
include(${AI_SHARE_PATH}/AiModules/Modules/PetRecognition/PetRecognition.cmake)
include(${AI_SHARE_PATH}/AiModules/Modules/Group1Detect/Group1Detect.cmake)
include(${AI_SHARE_PATH}/AiModules/Modules/Group2Detect/Group2Detect.cmake)
include(${AI_SHARE_PATH}/AiModules/Modules/Group3Detect/Group3Detect.cmake)
include(${AI_SHARE_PATH}/AiModules/Modules/Group4Detect/Group4Detect.cmake)
include(${AI_SHARE_PATH}/AiModules/Modules/Group5Detect/Group5Detect.cmake)
include(${AI_SHARE_PATH}/AiModules/Modules/PresonAttribute/PresonAttribute.cmake)
include(${AI_SHARE_PATH}/AiModules/Modules/VehicleAttribute/VehicleAttribute.cmake)
include(${AI_SHARE_PATH}/AiModules/Modules/NonMotorizedAttribute/NonMotorizedAttribute.cmake)

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
    SleepOnDutyDetect_Module
    TripDetect_Module
    ByteTrack_CPU
    YoloUltralytics
    Yolov5
    Yolov5Point
    YoloUltralyticsPoint
    Attribute
    MoveDetect_Module
    CameraObstruction_Module
    ImageFeature
    FaceDetect_Module
    FaceQualityAssessment_Module
    FaceAttribute_Module
    # ConstructionEncroachmentRoadDetect_Module
    # HelmetDetect_Module
    # ReflectiveDetect_Module
    # SmokingDetect_Module
    # FenceDetectDetect_Module
    # NPMNMDetect_Module
    # ElectricScooterDetect_Module
    # HumanAreaDetect_Module
    # FireDetect_Module
    # StreeDetect_Module
    # RubishDetect_Module
    # PlayPhoneDetect_Module
    # SafetyropeAndSoilDetect_Module
    # VehicleDetect_Module
    LicensePlateCognition_Module
    PetRecognition_Module
    Group1Detect_Module
    Group2Detect_Module
    Group3Detect_Module
    Group4Detect_Module
    Group5Detect_Module
    PresonAttribute_Module
    VehicleAttribute_Module
    NonMotorizedAttribute_Module
    libmqtt
    libcurl
)

# target_link_libraries(${PROJECT_NAME} 
#     ssl
#     crypto
# )

#TV-3882TI追加大模型所需库
if(DEVICE_TYPE STREQUAL "TV-3882TI")
    target_link_libraries(${PROJECT_NAME} 
        -lrockiva
        # libopencv
        RKVLM
    )
endif()

message("==========> Generate " ${PRO_NAME} " Makefile Success <==========")

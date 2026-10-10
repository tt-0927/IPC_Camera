# 版本限定
cmake_minimum_required(VERSION 3.12)

SET(PRO_NAME "stream")
project(${PRO_NAME})

# ipc_share
include(${IPC_SHARE_PATH}/ipc_share.cmake)
include(${IPC_SHARE_PATH}/common/utils/osd_font.cmake)
include(${IPC_SHARE_PATH}/protocols/tvsdk/tvsdk.cmake)

# AI 检测结果标准化与事件分发 Pipeline，ai_app 消费；
# 曾由 control/business/event/event.cmake 顺带引入，现由业务工程显式引入，
# 使不使用该模块的型号（工具链不支持 C++17）不参与编译
include(${IPC_SHARE_PATH}/event/event.cmake)

# cam_share
include(${CAM_SHARE_PATH}/include/include.cmake)
include(${CAM_SHARE_PATH}/data/frontEncode/frontEncode.cmake)
include(${CAM_SHARE_PATH}/data/osa_base/container/container.cmake)
include(${CAM_SHARE_PATH}/data/osa_base/ipc/ipc.cmake)
include(${CAM_SHARE_PATH}/data/osa_base/srt/srt.cmake)
include(${CAM_SHARE_PATH}/data/osa_base/networkEvent/networkEvent.cmake)
include(${CAM_SHARE_PATH}/communication/network/curl_http/curl_http.cmake)
include(${CAM_SHARE_PATH}/utils/curl/curl.cmake)
include(${CAM_SHARE_PATH}/device_system/get_time/get_time.cmake)
include(${CAM_SHARE_PATH}/device_system/system/system.cmake)
include(${CAM_SHARE_PATH}/common/common.cmake)
include(${CAM_SHARE_PATH}/common/log/nslog/nslog.cmake)
include(${CAM_SHARE_PATH}/file_processing/xml/xml.cmake)
include(${CAM_SHARE_PATH}/media/codec/audio/g711/codec_g711.cmake)
include(${CAM_SHARE_PATH}/media/calculate/calculate.cmake)
include(${CAM_SHARE_PATH}/utils/utils.cmake)

# hi_pipeline
include(${HI_PIPELINE_PATH}/hi_pipeline.cmake)

# ai_app
include(${CMAKE_CURRENT_LIST_DIR}/ai_app/ai_app.cmake)

# 源文件
set (MAIN_SRC_PATH
)

# 头文件
set (MAIN_INC_PATH
    #include
    ${SRC_ROOT_PATH}/include
)

foreach(item ${MAIN_SRC_PATH})
    aux_source_directory (${item} MAIN_LIST)
endforeach()

foreach(item ${MAIN_INC_PATH})
    include_directories (${item})
endforeach()

# 过滤
set(GREP_DIR "demo|Demo|ai_app|diagnostics")
# set(GREP_DIR "demo|Demo")

# 获取本程序子目录
execute_process(
    COMMAND find ${CMAKE_CURRENT_LIST_DIR}/  -type d 
    COMMAND grep -vE ${GREP_DIR} # 过滤目录 U
    OUTPUT_VARIABLE dirs_list
)

# message(${dirs_list})
string(REPLACE "\n" ";" dirs_list ${dirs_list})

# 添加所有源文件与头文件
foreach(item ${dirs_list})
    include_directories(${item})
    aux_source_directory(${item} STREAM_LIST)
endforeach()

add_executable (${PRO_NAME} ${MAIN_LIST} ${STREAM_LIST} ${SRC_LIST} ${HI_PIPELINE_LIST})

# 连接库：默认 smolrtsp；旧实现只在显式 live555 回滚时连接。
set(PROJECT_RTSP_LINK_LIBS)
if(IPC_RTSP_BACKEND STREQUAL "smolrtsp")
    if(NOT IPC_RTSP_LINK_LIBS)
        message(FATAL_ERROR "smolrtsp 后端未提供 IPC_RTSP_LINK_LIBS")
    endif()
    set(PROJECT_RTSP_LINK_LIBS ${IPC_RTSP_LINK_LIBS})
elseif(IPC_RTSP_BACKEND STREQUAL "live555")
    set(PROJECT_RTSP_LINK_LIBS libRtspServer)
else()
    message(FATAL_ERROR
        "IPC_RTSP_BACKEND 必须是 smolrtsp 或 live555，当前为：${IPC_RTSP_BACKEND}")
endif()

# WS 后端链接（IPC_WS_BACKEND 定义见 ipc_share/network/websocket/websocket.cmake）：
# evws 走平台 libevent 归档（core 含 evws/evhttp 符号，pthreads/openssl 为补充归档；
# smolrtsp 组合下 core 已随 RTSP 库传递，重复链接无害），lws 走动态库
if(IPC_WS_BACKEND STREQUAL "evws")
    set(PROJECT_WS_LINK_LIBS ipc_event_core ipc_event_pthreads ipc_event_openssl)
else()
    set(PROJECT_WS_LINK_LIBS libwebsockets)
endif()

# OSD 字体后端链接（IPC_OSD_FONT_BACKEND 定义见 ipc_share/common/utils/osd_font.cmake）：
# dotfont 零第三方库（dotfont.c 直接编入）；sdl 走 freetype/SDL2/SDL2_ttf 动态库
if(IPC_OSD_FONT_BACKEND STREQUAL "dotfont")
    set(PROJECT_OSD_FONT_LINK_LIBS "")
elseif(IPC_OSD_FONT_BACKEND STREQUAL "sdl")
    set(PROJECT_OSD_FONT_LINK_LIBS libfreetype libSDL2 libSDL2_ttf)
else()
    message(FATAL_ERROR
        "IPC_OSD_FONT_BACKEND 必须是 dotfont 或 sdl，当前为：${IPC_OSD_FONT_BACKEND}")
endif()

# 查看 platform 下的库文件 cmake
target_link_libraries(${PROJECT_NAME}
    -lpthread
    -latomic

    #mpp
    -lsecurec
    -lss_mpi
    -lss_mpi_sysmem
    -lss_mpi_sysbind
    -lss_mpi_isp
    -lss_mpi_ive
    -lot_mpi_isp
    -ldrc
    -ldehaze
    -lir_auto
    -lss_mpi_awb
    -lss_mpi_ae
    -lss_mpi_audio
    -lextend_stats
    -lupvqe
    -lcalcflicker
    -lbnr
    -ldnvqe
    -lvoice_engine
    -lldci
    -lacs
    -lvqe_res
    # -lvqe_record
    # -lvqe_talkv2
    # -lmp3_dec
    # -lmp3_enc
    # -lmp3_lame
    # -lopus
    -laac_comm
    -laac_dec
    -laac_enc
    -lss_mpi_cipher
    -lss_mpi_km
    -lot_mpi_otp
    #ai detect
    -lss_mpi_aidetect
    -lsvp_acl
    -lprotobuf-c
    -lsvp_aicpu
    -lss_ivs_md
    # -lrt
	# -lm
	# -ldl
	# -lpthread 
	# -lstdc++
	libz
	# third
	libxml2
    libzlog
    ${PROJECT_RTSP_LINK_LIBS}
    # libsrt
    libeXosip2
    libosip2
    libosipparser2
    libpugixml
    # libatomic
	libsqlite3
    libcurl
    # libgmssl
    libopenssl
	# -lMemoryCheck
    libupnp
    libsnmp++
    libagent++
    libiconv
    ${PROJECT_WS_LINK_LIBS}
    ${PROJECT_OSD_FONT_LINK_LIBS}
    # libopencv
    # libopencvfont
    # libtwolame
    # ZLMediaKit
    # libmk_api
    libonvif
    libffmpeg
    libNetTVSDKServer
    libmqtt

    # ai_share
    YoloUltralytics_rpn
    YoloUltralyticsPoint_rpn
    ImageFeature
)

# 在目标链接选项中添加（确保作用域清晰）
# target_link_options (
#     ${PRO_NAME} PRIVATE
#     -Wl,-rpath-link=${OPENSSL_LIBRARY_DIR}
# )

message("==========> Generate ${PRO_NAME} Makefile Success <==========")

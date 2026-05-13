# 版本限定
cmake_minimum_required(VERSION 3.12)

SET(PRO_NAME "stream")
project(${PRO_NAME})

# ipc_share
include(${IPC_SHARE_PATH}/ipc_share.cmake)
include(${IPC_SHARE_PATH}/common/utils/sdl/sdl.cmake)
# include(${IPC_SHARE_PATH}/protocols/tvsdk/tvsdk.cmake)

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

add_executable (${PRO_NAME} ${MAIN_LIST} ${STREAM_LIST} ${SRC_LIST} ${HI_PIPELINE_LIST})

#连接库
#查看platform下的库文件cmake
target_link_libraries(${PROJECT_NAME} 
    # -lpthread
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
    #ai detect
    -lss_mpi_aidetect
    -lsvp_acl
    -lprotobuf-c
    -lss_mpi_km
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
	libRtspServer
    # libsrt
    libeXosip2
    libosip2
    libosipparser2
    libpugixml
    # libatomic
	libsqlite3
    libgmssl
    libopenssl
	# -lMemoryCheck
    libupnp
    libsnmp++
    libagent++
    libiconv
    libwebsockets
    libfreetype
    libSDL2
    libSDL2_ttf
    # libopencv
    # libopencvfont
    # libtwolame
    # ZLMediaKit
    # libmk_api
    libonvif
    libffmpeg
    # libNetTVSDKServer

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

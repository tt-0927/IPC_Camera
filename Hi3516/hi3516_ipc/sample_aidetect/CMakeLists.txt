# 版本限定
cmake_minimum_required(VERSION 3.12)

SET(PRO_NAME "sample_aidetect")
project(${PRO_NAME})

include(${IPC_SHARE_PATH}/ret/ret.cmake)

# 源文件
set (MAIN_SRC_PATH
    # cam_share
    ${CAM_SHARE_PATH}/device_system/get_time
    ${CAM_SHARE_PATH}/utils/cpp_base/performance_monitor

    # HiMpp
    ${HI_PIPELINE_PATH}/common
    ${HI_MPP_PATH}/cbb/audio/adp/src
    ${HI_MPP_PATH}/cbb/isp/user/sensor/hi3516cv610/common
    ${HI_MPP_PATH}/cbb/isp/user/sensor/hi3516cv610/smart_sc4336p
    ${HI_MPP_PATH}/cbb/isp/user/sensor/hi3516cv610/smart_sc500ai
)

# 头文件
set (MAIN_INC_PATH
    #include
    # ${SRC_ROOT_PATH}/include
    # cam_share
    ${CAM_SHARE_PATH}/device_system/get_time
    ${CAM_SHARE_PATH}/utils/cpp_base/performance_monitor

    # HiMpp
    ${HI_PIPELINE_PATH}/common
    ${HI_MPP_PATH}/include
    ${HI_MPP_PATH}/cbb/audio/adp/include
    ${HI_MPP_PATH}/cbb/isp/include
    ${HI_MPP_PATH}/cbb/isp/user/3a/include
    ${HI_MPP_PATH}/cbb/isp/user/sensor/hi3516cv610/common
    ${HI_MPP_PATH}/cbb/isp/user/sensor/hi3516cv610/smart_sc4336p
    ${HI_MPP_PATH}/cbb/isp/user/sensor/hi3516cv610/smart_sc500ai
)

foreach(item ${MAIN_SRC_PATH})
    aux_source_directory (${item} MAIN_LIST)
endforeach()

foreach(item ${MAIN_INC_PATH})
    include_directories (${item})
endforeach()

# 过滤
set(GREP_TEXT "demo|Demo")

# 获取本程序子目录
execute_process(
    COMMAND find ${CMAKE_CURRENT_LIST_DIR}/  -type d 
    COMMAND grep -vE ${GREP_TEXT} # 过滤目录 U
    OUTPUT_VARIABLE dirs_list
)

message(${dirs_list})
string(REPLACE "\n" ";" dirs_list ${dirs_list})

# 添加所有源文件与头文件
foreach(item ${dirs_list})
    include_directories(${item})
    aux_source_directory(${item} STREAM_LIST)
endforeach()

add_executable (${PRO_NAME} ${MAIN_LIST} ${STREAM_LIST} ${SRC_LIST})

#连接库
#查看platform下的库文件cmake
target_link_libraries(${PROJECT_NAME} 
    # -lpthread

    #mpp
    -lsecurec
    -lss_mpi
    -lss_mpi_sysmem
    -lss_mpi_sysbind
    -lss_mpi_isp
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
    -lvqe_record
    -lmp3_dec
    -lmp3_enc
    -lmp3_lame
    -lopus
    -laac_comm
    -laac_dec
    -laac_enc
    #ai detect
    -lss_mpi_aidetect
    -lsvp_acl
    -lprotobuf-c

	# third
	libz
)

message("==========> Generate ${PRO_NAME} Makefile Success <==========")

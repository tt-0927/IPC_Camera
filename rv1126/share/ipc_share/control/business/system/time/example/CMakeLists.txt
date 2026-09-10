# 版本限定
cmake_minimum_required(VERSION 3.16.2)

# 程序名称
SET(PRO_NAME "ntp_example")
project(${PRO_NAME})

# 头文件
set (INCLUDE_PATH
    # main
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../ntp
    # ipc_share
    ${IPC_SHARE_PATH}/common/define
    ${IPC_SHARE_PATH}/common/convert
    ${IPC_SHARE_PATH}/ret
    ${IPC_SHARE_PATH}/design
    ${IPC_SHARE_PATH}/logger
    ${IPC_SHARE_PATH}/data_format/json
    ${IPC_SHARE_PATH}/shared_library/json/cJSON
    # cam_share
    ${CAM_SHARE_PATH}/common/log
    ${CAM_SHARE_PATH}/device_system/get_time
)

# 源文件
set (SOURCE_PATH
    # main
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../ntp
    # ipc_share
    ${IPC_SHARE_PATH}/common/convert
    ${IPC_SHARE_PATH}/ret
    ${IPC_SHARE_PATH}/design
    ${IPC_SHARE_PATH}/logger
    ${IPC_SHARE_PATH}/data_format/json
    ${IPC_SHARE_PATH}/shared_library/json/cJSON
    # cam_share
    ${CAM_SHARE_PATH}/common/log
    ${CAM_SHARE_PATH}/device_system/get_time
)

# 库文件
set (LIBS
    # sdk
    libsdk
    # system
    -lrt
	-lm
	-ldl
    -lpthread 
    -lstdc++fs
)

# 添加头文件
foreach(item ${INCLUDE_PATH})
    include_directories (${item})
endforeach()

# 添加源文件
foreach(item ${SOURCE_PATH})
    aux_source_directory (${item} SOURCE_LIST)
endforeach()

# 编译
add_executable (${PRO_NAME} ${SOURCE_LIST})

# 链接库
target_link_libraries (${PRO_NAME} ${LIBS})

message("==========> Generate " ${PRO_NAME} " Makefile Success <==========")

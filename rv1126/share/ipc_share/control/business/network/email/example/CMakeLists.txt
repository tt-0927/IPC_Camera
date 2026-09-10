# 版本限定
cmake_minimum_required(VERSION 3.16.2)

# 程序名称
SET(PRO_NAME "email_example")
project(${PRO_NAME})

# 引用cmake
# main
message(${SRC_ROOT_PATH})
include(${SRC_ROOT_PATH}/main_app/main_app.cmake)
# ipc_share
message(${IPC_SHARE_PATH})
include(${IPC_SHARE_PATH}/ipc_share.cmake)
# cam_share
message(${CAM_SHARE_PATH})
include(${CAM_SHARE_PATH}/cam_share.cmake)
# rk_pipeline
message(${RK_PIPELINE_PATH})
include(${RK_PIPELINE_PATH}/rk_pipeline.cmake)

# 头文件
set (INCLUDE_PATH
    # root
    ${SRC_ROOT_PATH}/include
)

# 源文件
set (SOURCE_PATH
    # main
    ${CMAKE_CURRENT_LIST_DIR}
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
    -lfreetype
    -liconv
    # third
    libxml2
    libx264
    libx265
    libssl
    libcrypto
    libsrt
    libfaac
    libfdk-aac
    libffmpeg
    libzlog
    libwebsockets
    libsqlite3
    libeXosip2
    libosip2
    libosipparser2
    libpugixml
    libupnp
    libsnmp++
    libagent++
    libRtspServer
    libonvif
    librtmp
    # rk_pipeline
    -lrockit
    -lrkaiq
    -lrknnmrt
    -lrockiva
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
add_executable (${PRO_NAME} ${SOURCE_LIST} ${SRC_LIST})

# 在目标链接选项中添加（确保作用域清晰）
target_link_options (
    ${PRO_NAME} PRIVATE
    -Wl,-rpath-link=${ROCKIT_LIBRARY_DIR}
)

# 链接库
target_link_libraries (${PRO_NAME} ${LIBS})

message("==========> Generate " ${PRO_NAME} " Makefile Success <==========")

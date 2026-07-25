# 版本限定
cmake_minimum_required(VERSION 3.12)

SET(PRO_NAME "upgrade")
project(${PRO_NAME})

# 引用cmake
include(${IPC_SHARE_PATH}/common/common.cmake)
include(${IPC_SHARE_PATH}/design/design.cmake)
include(${IPC_SHARE_PATH}/ret/ret.cmake)
include(${IPC_SHARE_PATH}/encrypt/md5/md5.cmake)
include(${IPC_SHARE_PATH}/network/tcp/tcp.cmake)
include(${IPC_SHARE_PATH}/data_format/json/json.cmake)
# include(${IPC_SHARE_PATH}/network/network.cmake)
include(${CAM_SHARE_PATH}/common/log/log.cmake)
include(${CAM_SHARE_PATH}/utils/cpp_base/list/list.cmake)
include(${IPC_SHARE_PATH}/standard/standard.cmake)

# 头文件
set (INCLUDE_PATH
    #include
    ${SRC_ROOT_PATH}/include

    ${CMAKE_CURRENT_LIST_DIR}/common
    ${CMAKE_CURRENT_LIST_DIR}/communication
    ${CMAKE_CURRENT_LIST_DIR}/manage
    ${IPC_SHARE_PATH}/network

    ${CAM_SHARE_PATH}/utils/cpp_base
)

# 源文件
set (SOURCE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/communication
    ${CMAKE_CURRENT_LIST_DIR}/manage

    ${CAM_SHARE_PATH}/utils/cpp_base
)

# 库文件
set (LIBS
    # system
    # -lrt
	# -lm
	# -ldl
    -lpthread
    libz
    libzlog
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

# 链接库
target_link_libraries (${PRO_NAME} ${LIBS})

message("==========> Generate " ${PRO_NAME} " Makefile Success <==========")

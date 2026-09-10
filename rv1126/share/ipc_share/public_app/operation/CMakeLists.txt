# 版本限定
cmake_minimum_required(VERSION 3.12)

# 设置程序名称
SET(PRO_NAME "operation")
project(${PRO_NAME})

include(${IPC_SHARE_PATH}/data_format/data_format.cmake)
include(${IPC_SHARE_PATH}/standard/standard.cmake)
include(${IPC_SHARE_PATH}/design/design.cmake)
include(${IPC_SHARE_PATH}/control/database/database.cmake)
include(${IPC_SHARE_PATH}/ret/ret.cmake)
include(${IPC_SHARE_PATH}/common/common.cmake)
include(${IPC_SHARE_PATH}/network/tcp/tcp.cmake)
include(${IPC_SHARE_PATH}/network/uds/uds.cmake)
# include(${IPC_SHARE_PATH}/network/udp/udp.cmake)
# include(${IPC_SHARE_PATH}/network/uds/uds.cmake)
# include(${IPC_SHARE_PATH}/network/network.cmake)
include(${IPC_SHARE_PATH}/control/business/network/https/ca/ca.cmake)
include(${IPC_SHARE_PATH}/data_structure/list/list.cmake)


include(${CAM_SHARE_PATH}/common/common.cmake)
include(${CAM_SHARE_PATH}/communication/mqtt/mqtt.cmake)
include(${CAM_SHARE_PATH}/communication/network/curl_http/curl_http.cmake)
include(${CAM_SHARE_PATH}/utils/curl/curl.cmake)

# 头文件
set (INCLUDE_PATH
    #include
    ${SRC_ROOT_PATH}/include
)

# 源文件
set (SOURCE_PATH
)

# 添加头文件
foreach(item ${INCLUDE_PATH})
    include_directories (${item})
endforeach()

# 添加源文件
foreach(item ${SOURCE_PATH})
    aux_source_directory (${item} SOURCE_LIST)
endforeach()

SET(LIBS
	-lstdc++
	-lpthread
    -latomic
)
set (LOCAL_LIBS
    libopenssl
	libsqlite3
	libcurl
    libmqtt
    libz
)

# 过滤
set(GREP_TEXT "demo")

# 获取本程序子目录
execute_process(
    COMMAND find ${CMAKE_CURRENT_LIST_DIR}/  -type d
    COMMAND grep -vE ${GREP_TEXT} # 过滤目录 U
    OUTPUT_VARIABLE dirs_list
    )
string(REPLACE "\n" ";" dirs_list ${dirs_list})

# 添加所有源文件与头文件
foreach(item ${dirs_list})
    include_directories (${item})
    aux_source_directory (${item} SRC_LIST)
endforeach()

# 编译
add_executable(${PROJECT_NAME} ${SOURCE_LIST} ${SRC_LIST} )

# 链接库路径
target_link_libraries(${PROJECT_NAME} ${LIBS} ${LOCAL_LIBS})

message("==========> 生成 [" ${PROJECT_NAME} "] Makefile文件 成功 <==========")

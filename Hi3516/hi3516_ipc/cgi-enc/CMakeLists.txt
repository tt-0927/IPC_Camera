# 版本限定
cmake_minimum_required(VERSION 3.10.2)

SET(PRO_NAME "encoder.cgi")
project(${PRO_NAME})

include(${IPC_SHARE_PATH}/common/common.cmake)
include(${IPC_SHARE_PATH}/design/design.cmake)
include(${IPC_SHARE_PATH}/data_format/json/json.cmake)
include(${IPC_SHARE_PATH}/ret/ret.cmake)

include(${CAM_SHARE_PATH}/data/osa_base/file/file.cmake)
include(${CAM_SHARE_PATH}/include/include.cmake)
include(${CAM_SHARE_PATH}/utils/cpp_base/list/list.cmake)

# 添加头文件
set(INCLUDE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/
	# ${CMAKE_CURRENT_SOURCE_DIR}/include/
	${CMAKE_CURRENT_SOURCE_DIR}/../include/

	${CAM_SHARE_PATH}/
	${CAM_SHARE_PATH}/common/log/
	${CAM_SHARE_PATH}/communication/network/shortLink/
	${CAM_SHARE_PATH}/communication/socket/
	${CAM_SHARE_PATH}/utils/cpp_base/
)

# 添加源文件
set(SRC_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/

	${CAM_SHARE_PATH}/
	${CAM_SHARE_PATH}/common/log/
	${CAM_SHARE_PATH}/communication/network/shortLink/
	${CAM_SHARE_PATH}/communication/socket/
	${CAM_SHARE_PATH}/utils/cpp_base/
)

SET(LIBS
	-lpthread
	libfcgi
)

# 添加本程序的头文件
foreach(item ${INCLUDE_PATH})
	include_directories(${item})
endforeach()

# 添加本程序的源文件
foreach(item ${SRC_PATH})
	aux_source_directory(${item} SRC_LIST)
endforeach()

# 编译
add_executable(${PRO_NAME} ${SRC_LIST})

# 链接库路径
target_link_libraries(${PRO_NAME} ${LIBS} ${LOCAL_LIBS})

message("==========> Generate " ${PRO_NAME} " Makefile Success <==========")

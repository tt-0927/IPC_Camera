# 版本限定
cmake_minimum_required(VERSION 3.12)

SET(PRO_NAME "daemon")
project(${PRO_NAME})

# 引用cmake
include(${CAM_SHARE_PATH}/common/log/log.cmake)
include(${IPC_SHARE_PATH}/common/timezone/timezone.cmake)

# 头文件
set (INCLUDE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${IPC_SHARE_PATH}/common/define
    ${IPC_SHARE_PATH}/ret
)

# 源文件
set (SOURCE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)

# 库文件
set (LIBS
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

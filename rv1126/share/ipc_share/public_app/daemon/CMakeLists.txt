# 版本限定
cmake_minimum_required(VERSION 3.12)

SET(PRO_NAME "daemon")
project(${PRO_NAME})

# 引用cmake
include(${CAM_SHARE_PATH}/common/log/log.cmake)
include(${IPC_SHARE_PATH}/common/timezone/timezone.cmake)

# 本地 SD 存储能力门控（设备画像 IPC_CAP_LOCAL_STORAGE，0/1）。
# daemon 不 include ipc_share.cmake，拿不到共享层顶部的兜底与宏导出，
# 在此局部兜底并导出带值宏（先例：record 进程的 IPC_CAP_RECORD_*）。
if(NOT DEFINED IPC_CAP_LOCAL_STORAGE)
    set(IPC_CAP_LOCAL_STORAGE 1)
endif()
add_compile_definitions(CAP_LOCAL_STORAGE=${IPC_CAP_LOCAL_STORAGE})

# 头文件
set (INCLUDE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${IPC_SHARE_PATH}/common/define
    ${IPC_SHARE_PATH}/common/utils
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

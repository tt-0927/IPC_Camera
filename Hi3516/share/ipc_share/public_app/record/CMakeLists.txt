# 版本限定
cmake_minimum_required(VERSION 3.12)

SET(PRO_NAME "record")
# 设置程序名称
project(${PRO_NAME})

# include(${IPC_SHARE_PATH}/ipc_share.cmake)
include(${IPC_SHARE_PATH}/common/common.cmake)
include(${IPC_SHARE_PATH}/ret/ret.cmake)
include(${IPC_SHARE_PATH}/data_format/json/json.cmake)
# include(${IPC_SHARE_PATH}/standard/standard.cmake)
include(${IPC_SHARE_PATH}/design/design.cmake)
# include(${IPC_SHARE_PATH}/data_structure/data_structure.cmake)
# include(${IPC_SHARE_PATH}/media/media.cmake)
include(${IPC_SHARE_PATH}/network/tcp/tcp.cmake)
include(${IPC_SHARE_PATH}/network/uds/uds.cmake)
include(${IPC_SHARE_PATH}/design/design.cmake)
include(${IPC_SHARE_PATH}/data_structure/que/que.cmake)
include(${IPC_SHARE_PATH}/data_structure/list/list.cmake)

# record 额外 include
if(NOT DEFINED IPC_CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE)
    set(IPC_CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE 0)
endif()
# record 链接 fdk-aac
if(NOT DEFINED IPC_CAP_RECORD_LINK_FDK_AAC)
    set(IPC_CAP_RECORD_LINK_FDK_AAC 0)
endif()

#源文件
set (RECORD_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/common
    ${CMAKE_CURRENT_LIST_DIR}/communication
    ${CMAKE_CURRENT_LIST_DIR}/format
    ${CMAKE_CURRENT_LIST_DIR}/manage
    ${CMAKE_CURRENT_LIST_DIR}/record_file

    # cam_share
    ${CAM_SHARE_PATH}/common/log
    ${CAM_SHARE_PATH}/common/log/nslog
    ${CAM_SHARE_PATH}/utils/cpp_base
)

foreach(item ${RECORD_PATH})
    aux_source_directory (${item} RECORD_LIST)
endforeach()

#头文件
set (RECORD_INCLUDE
    #include
    ${SRC_ROOT_PATH}/include

    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/common
    ${CMAKE_CURRENT_LIST_DIR}/communication
    ${CMAKE_CURRENT_LIST_DIR}/format
    ${CMAKE_CURRENT_LIST_DIR}/manage
    ${CMAKE_CURRENT_LIST_DIR}/record_file

    # cam_share
    ${CAM_SHARE_PATH}/common/log
    ${CAM_SHARE_PATH}/common/log/nslog
    ${CAM_SHARE_PATH}/utils/cpp_base
)
# record 额外 include
if(IPC_CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE)
    list(APPEND RECORD_INCLUDE ${CAM_SHARE_PATH}/include)
endif()
foreach(item ${RECORD_INCLUDE})
    include_directories ( ${item} ) 
endforeach()

add_compile_options( -Wall )

add_executable (${PRO_NAME} ${RECORD_LIST} ${SRC_LIST})

# record 链接 fdk-aac
if(IPC_CAP_RECORD_LINK_FDK_AAC)
    target_link_options(${PRO_NAME} PRIVATE
        "-Wl,--rpath-link,${IPC_PLATFORM_PATH}/lib/fdk-aac/lib"
        "-L${IPC_PLATFORM_PATH}/lib/fdk-aac/lib"
    )
endif()

# 库名仅 881T/882TI 才链接
set(_extra_libs)
if(IPC_CAP_RECORD_LINK_FDK_AAC)
    list(APPEND _extra_libs fdk-aac)
endif()

#链接库
#查看platform下的库文件cmake
target_link_libraries(${PRO_NAME}
    -lpthread
    -static-libstdc++
    -lstdc++
    # -latomic

    # third
    libffmpeg
    libzlog
    ${_extra_libs}
)

message("==========> 生成 [ ${PRO_NAME} ] Makefile文件 成功 <==========")

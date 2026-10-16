# VENC JPEG 用户态批量抠图（send_frame_batch_crop）可用性 PoC
# 构建方式：在 hi3516_ipc/CMakeLists.txt 中打开 add_subdirectory(test_venc_batch_crop) 后随固件构建
cmake_minimum_required(VERSION 3.12)

set(PRO_NAME "test_venc_batch_crop")
project(${PRO_NAME})

include(${IPC_SHARE_PATH}/ret/ret.cmake)

# 头文件：MPP SDK 头文件目录
include_directories(
    ${HI_MPP_PATH}/include
)

# 源文件
aux_source_directory(${CMAKE_CURRENT_LIST_DIR} POC_SRC_LIST)

add_executable(${PRO_NAME} ${POC_SRC_LIST} ${SRC_LIST})

# 链接库：仅依赖 MPP 基础库（VENC/VB/SYS），不依赖 hi_pipeline 封装，验证的是纯 MPP 能力
target_link_libraries(${PRO_NAME}
    -lsecurec
    -lss_mpi
    -lss_mpi_sysmem
    -lss_mpi_sysbind
    -lpthread
)

message("==========> Generate ${PRO_NAME} Makefile Success <==========")

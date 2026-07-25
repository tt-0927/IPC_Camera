# 版本限定
cmake_minimum_required(VERSION 3.12)

# 设置程序名称
project("ZipFormer_Demo")

# NPU_ARM_64
set(CROSS_PREFIX "aarch64-buildroot-linux-gnu-")
set(Framework rockchip_arm_64)
# NPU_ARM_32
# set(CROSS_PREFIX "arm-linux-gnueabihf-")
# set(Framework rockchip_arm_32)

# 编译链设置
string(STRIP "${CROSS_PREFIX}" CROSS_PREFIX)
# 根据前缀自动设置编译工具
set(CMAKE_C_COMPILER    "${CROSS_PREFIX}gcc")
set(CMAKE_CXX_COMPILER  "${CROSS_PREFIX}g++")
set(CMAKE_AR            "${CROSS_PREFIX}ar")
set(CMAKE_RANLIB        "${CROSS_PREFIX}ranlib")
set(CMAKE_LINKER        "${CROSS_PREFIX}ld")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g") 

# 推理依赖
include(${CMAKE_CURRENT_LIST_DIR}/../../../../../ai_model/${Framework}/ai/ai.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../RK/ASR/ZipFormer/ZipFormer.cmake)

# 添加所有源文件与头文件
set(dirs_list 
    # 预处理
    ${CMAKE_CURRENT_LIST_DIR}/../../PreProcess/AV
    ${CMAKE_CURRENT_LIST_DIR}/../../PreProcess/AV/kissfft
    ${CMAKE_CURRENT_LIST_DIR}/../../PreProcess/AV/kaldi-native-fbank/csrc
    
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/network/tcp/sdk_net_base/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/common/error/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/log/dlog/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/system/time/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/data_structure/list/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/network/common/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../bl_share/data_format/xml/
)

# 添加共享库的文件
foreach(item ${dirs_list})
	include_directories(${item})
    aux_source_directory(${item} OS_SHARE_SRC_LIST)
endforeach()

SET(LIBS
	-lpthread
	-lxml2
    ZipFormer
)

# 设置生成文件存放路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../output/bin)

# 编译
add_executable(${PROJECT_NAME} ${OS_SHARE_SRC_LIST} main.cpp)

# 链接库路径
target_link_libraries(${PROJECT_NAME}
    ${LIBS}
)



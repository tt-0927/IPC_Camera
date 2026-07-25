# 版本限定
cmake_minimum_required(VERSION 3.10.2)

# 设置程序名称
project("BridgeAlgorithmDemo")

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
SET(CMAKE_C_COMPILER "aarch64-buildroot-linux-gnu-gcc")
SET(CMAKE_CXX_COMPILER "aarch64-buildroot-linux-gnu-g++")
SET(CMAKE_AR "aarch64-buildroot-linux-gnu-ar")
SET(CMAKE_LINKER "aarch64-buildroot-linux-gnu-ld")

# 设置C++标准为C++17，如果编译器支持的话
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 检查编译器是否支持C++17
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++17" COMPILER_SUPPORTS_CXX17)

if(NOT COMPILER_SUPPORTS_CXX17)
    message(FATAL_ERROR "编译器不支持C++17")
endif()


include_directories(${CMAKE_CURRENT_LIST_DIR}/../../../../ai/opencv4.5.5/include/opencv4)


link_directories(${CMAKE_CURRENT_LIST_DIR}/../../../../../0663c/third-party/lib/rk3588/link | ${CMAKE_CURRENT_LIST_DIR}/../../../../../0663c/third-party/lib/rk3588/staticso)
#封装opencVda库路径
set( SRC_LIB_PATH
    ${CMAKE_CURRENT_LIST_DIR}/../../../../ai/lib/
)
foreach(item ${SRC_LIB_PATH})
    link_directories( ${item} )
endforeach()
# 文件
set (LIBS 
    -lpthread 
    -lopencv_core
    -lopencv_highgui
    -lopencv_imgcodecs
    -lopencv_imgproc
    -lopencv_videoio
    -lopencv_font

    -lrknnrt
    -lasound
    -ldrm_rockchip
    -ldrm
    -lrockchip_mpp
    -lrockit
)


# 文件
set (SRC_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..

    ${CMAKE_CURRENT_LIST_DIR}/../../../../log/dlog/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../common/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../common/error/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../ai/rk_include/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../data_structure/que/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../system/time/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../standard/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../unclassified/os/

    ${CMAKE_CURRENT_LIST_DIR}/../../../../../rockit/include
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../rockit/ao/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../rockit/ai/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../rockit/aenc/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../rockit/adec/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../rockit/common/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../rockit/rgn/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../rockit/syn/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../rockit/tde/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../rockit/vdec/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../rockit/venc/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../rockit/vo/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../rockit/vi/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../rockit/vpss/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../rockit/vgs/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../rockit/im2d_api/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../rockit/im2d_api/include/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../rockit/drm/include/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../rockit/drm/include/libdrm/
)

foreach(item ${SRC_PATH})
    include_directories ( ${item} ) 
    aux_source_directory (${item} SRC_LIST)
endforeach()

include_directories(${CMAKE_CURRENT_LIST_DIR}/../../../../media/image/)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../BridgeAlgorithm BridgeAlgorithmBuild)

 # 编译
 add_executable (${PROJECT_NAME} ${SRC_LIST}) 

 # 链接库路径
 target_link_libraries (${PROJECT_NAME} ${LIBS} BridgeAlgorithm)
 
 message("==========> 生成 [" ${PROJECT_NAME} "] Makefile文件 成功 <==========")

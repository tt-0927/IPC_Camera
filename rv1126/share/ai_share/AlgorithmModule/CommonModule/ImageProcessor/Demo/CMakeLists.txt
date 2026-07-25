# 版本限定
cmake_minimum_required(VERSION 3.10.2)

# 设置程序名称
project("ImageProcessorDemo")

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
SET(CMAKE_C_COMPILER "aarch64-buildroot-linux-gnu-gcc")
SET(CMAKE_CXX_COMPILER "aarch64-buildroot-linux-gnu-g++")
SET(CMAKE_AR "aarch64-buildroot-linux-gnu-ar")
SET(CMAKE_LINKER "aarch64-buildroot-linux-gnu-ld")

# 设置C++标准
include(CheckCXXCompilerFlag)

check_cxx_compiler_flag("-std=c++17" COMPILER_SUPPORTS_CXX17)
if(COMPILER_SUPPORTS_CXX17)
    add_compile_options("-std=c++17")
else()
    message(FATAL_ERROR "编译器不支持C++17")
endif()


include_directories(${CMAKE_CURRENT_LIST_DIR}/../../../../../ai/opencv4.5.5/include/opencv4)

link_directories(${CMAKE_CURRENT_LIST_DIR}/../../../../../../0663c/third-party/lib/rk3588/link)

# 文件
set (LIBS 
    -lpthread 
    -lopencv_core
    -lopencv_highgui
    -lopencv_imgcodecs
    -lopencv_imgproc
    -lopencv_videoio
    -lopencv_font
)

# 文件
set (SRC_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../log/dlog/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../common/error/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../ai/opencv4.5.5/include/opencv4/
)

foreach(item ${SRC_PATH})
    include_directories ( ${item} ) 
    aux_source_directory (${item} SRC_LIST)
endforeach()

include_directories (${CMAKE_CURRENT_LIST_DIR}/../../../../../media/image/)

 # 编译
 add_executable (${PROJECT_NAME} ${SRC_LIST}) 

 # 链接库路径
 target_link_libraries (${PROJECT_NAME} ${LIBS})
 
 message("==========> 生成 [" ${PROJECT_NAME} "] Makefile文件 成功 <==========")

# 版本限定
cmake_minimum_required(VERSION 3.10.2)

# 设置程序名称
project("InferenceDemo")

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

set(PROJECT_LIB_PATH ${CMAKE_CURRENT_LIST_DIR}/../../../../../0663c/third-party/lib/rk3588/)
message("==========> 当前编译库路径: ${PROJECT_LIB_PATH}")
link_directories(${PROJECT_LIB_PATH} | ${PROJECT_LIB_PATH}/staticso)

# 文件
set (SRC_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)

foreach(item ${SRC_PATH})
    include_directories ( ${item} ) 
    aux_source_directory (${item} SRC_LIST)
endforeach()


add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../RK RkModelBuild)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../CPU CPUModelBuild)

 # 编译
 add_executable (${PROJECT_NAME} ${SRC_LIST}) 

 # 链接库路径
 target_link_libraries (${PROJECT_NAME} ${LIBS} aiInferenceModel_RK aiInferenceModel_CPU)
 
 message("==========> 生成 [" ${PROJECT_NAME} "] Makefile文件 成功 <==========")
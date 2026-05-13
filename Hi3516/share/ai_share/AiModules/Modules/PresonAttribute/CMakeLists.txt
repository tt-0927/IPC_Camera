# 版本限定
cmake_minimum_required(VERSION 3.10.2)

set(MODULE_NAME "PresonAttribute_Module")

# 设置C++标准为C++11, 如果编译器支持的话
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 检查编译器是否支持C++11
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)

if(NOT COMPILER_SUPPORTS_CXX11)
    message(FATAL_ERROR "编译器不支持C++11")
endif()

# 必须在前面
add_library(${MODULE_NAME} STATIC)

# 判断opencv环境是否设置
if(NOT DEFINED OPENCV_ENV OR OPENCV_ENV STREQUAL "")
    message(FATAL_ERROR "Error: OPENCV_ENV 参数异常")
endif()

# 判断opencv环境是否设置
if(NOT DEFINED ROCKIT_ENV OR ROCKIT_ENV STREQUAL "")
    message(FATAL_ERROR "Error: ROCKIT_ENV 参数异常")
endif()

# 判断rockit头文件是否设置
if(NOT DEFINED ROCKIT_INCLUDE OR ROCKIT_INCLUDE STREQUAL "")
    message(FATAL_ERROR "Error: ROCKIT_INCLUDE 参数异常")
endif()

# 添加模块
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../Inference/RK/Classification/PresonAttribute ${CMAKE_CURRENT_BINARY_DIR}/../output/cmake_binary/PresonAttribute)

target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../Common/)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../Common/ MODULE_SOURCES)

# 过滤
set(GREP_TEXT "demo|Demo")

# 获取本程序子目录
execute_process(
    COMMAND find ${CMAKE_CURRENT_LIST_DIR}/  -type d 
    COMMAND grep -vE ${GREP_TEXT} # 过滤目录 U
    OUTPUT_VARIABLE dirs_list
    )
string(REPLACE "\n" ";" dirs_list ${dirs_list})
# 添加所有源文件与头文件
foreach(item ${dirs_list})
    target_include_directories(${MODULE_NAME} PUBLIC ${item})
    aux_source_directory (${item} MODULE_SOURCES)
endforeach() 

# 设置的源文件
target_sources(${MODULE_NAME} PRIVATE ${MODULE_SOURCES})

# 链接库
target_link_libraries(${MODULE_NAME} PUBLIC 
    PresonAttribute_RK
)

# 打印消息，指示模块设置成功
message("==========> [${PROJECT_NAME}] 使用[${MODULE_NAME}]模块 <==========")   

# 版本限定
cmake_minimum_required(VERSION 3.10.2)

set(MODULE_NAME "aiInferenceModel_RK")

# 设置C++标准为C++17, 如果编译器支持的话
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 检查编译器是否支持C++17
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++17" COMPILER_SUPPORTS_CXX17)

if(NOT COMPILER_SUPPORTS_CXX17)
    message(FATAL_ERROR "编译器不支持C++17")
endif()

# 必须在前面
add_library(${MODULE_NAME} STATIC)

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
    # -ldrm_rockchip
    -ldrm
    -lrockchip_mpp
    -lrockit
)


# 文件  
set (SRC_PATH
    ${CMAKE_CURRENT_LIST_DIR}/../
    ${CMAKE_CURRENT_LIST_DIR}/../../
    ${CMAKE_CURRENT_LIST_DIR}/../../../opencv4.5.5/include/opencv4/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../bl_share/log/dlog/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../bl_share/system/time
    ${CMAKE_CURRENT_LIST_DIR}/../../../rk_include/
    ${CMAKE_CURRENT_LIST_DIR}/../PostProcess/

)
foreach(item ${SRC_PATH})
    target_include_directories(${MODULE_NAME} PUBLIC ${item})
    aux_source_directory(${item} MODULE_SOURCES)
endforeach()

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

# 链接库路径
target_link_libraries(${MODULE_NAME} PUBLIC ${LIBS})

message("==========> [" ${PROJECT_NAME} "] 使用[" ${MODULE_NAME} "]模块 <==========")


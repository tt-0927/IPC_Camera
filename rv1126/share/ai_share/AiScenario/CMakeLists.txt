# 版本限定
cmake_minimum_required(VERSION 3.10.2)

set(MODULE_NAME "AI_SCENARIO")

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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# 必须在前面
add_library(${MODULE_NAME} STATIC)

# 添加模块
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Inference/CPU)

if (PLATFORM STREQUAL "RK")
    # 添加 RK 平台特定模块的子目录
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Inference/RK)

    # 定义 RK 平台的库
    set(PLATFORM_LIBS
        aiInferenceModel_RK
    )
    
    # 设置是否使用本地 AI 代码的选项
    set(USE_LOCAL_AI ON)
    
    # 添加预处理定义，根据选项设置
    if(USE_LOCAL_AI)
        add_compile_definitions(USE_LOCAL_AI)
    endif()

    # 输出消息，确认平台类型和 USE_LOCAL_AI 的值
    message("==========> [${PROJECT_NAME}] 平台类型[${PLATFORM}] <==========")
else()  
    # 如果未知的平台，输出错误消息并终止 CMake 过程
    message(FATAL_ERROR "未知的平台: ${PLATFORM}")
endif()

if(USE_LOCAL_AI)
    # 文件
    set (LIBS 
        -lpthread 
        -lopencv_core
        -lopencv_highgui
        -lopencv_imgcodecs
        -lopencv_imgproc
        -lopencv_videoio
        -lopencv_font
        -lfreetype
        -lzlog
    )
else() 
    message("==========> [${PROJECT_NAME}]-[${MODULE_NAME}]-[${USE_LOCAL_AI}] 不使用本地AI分析, 不设置库文件 <==========")
endif()

# 文件
set (SRC_PATH
    ${CMAKE_CURRENT_LIST_DIR}/../opencv4.5.5/include/opencv4/

    ${CMAKE_CURRENT_LIST_DIR}/../../bl_share/log/dlog/   
    ${CMAKE_CURRENT_LIST_DIR}/../../bl_share/common/
    ${CMAKE_CURRENT_LIST_DIR}/../../bl_share/common/error/
    ${CMAKE_CURRENT_LIST_DIR}/../../bl_share/data_structure/que/
    ${CMAKE_CURRENT_LIST_DIR}/../../bl_share/system/time/
    ${CMAKE_CURRENT_LIST_DIR}/../../bl_share/standard/
    ${CMAKE_CURRENT_LIST_DIR}/../../bl_share/unclassified/os/
    ${CMAKE_CURRENT_LIST_DIR}/../../bl_share/data_format/json/
    ${CMAKE_CURRENT_LIST_DIR}/../../bl_share/media/image/
    ${CMAKE_CURRENT_LIST_DIR}/../../bl_share/log/nslog/
    ${CMAKE_CURRENT_LIST_DIR}/../../bl_share/network/common/
)
foreach(item ${SRC_PATH}) 
    target_include_directories(${MODULE_NAME} PUBLIC ${item})
    aux_source_directory(${item} MODULE_SOURCES)
endforeach()

if(USE_LOCAL_AI)
    # 过滤
    set(GREP_TEXT "demo|Demo|Inference")

    # 获取本程序子目录
    execute_process(
        COMMAND find ${CMAKE_CURRENT_LIST_DIR}/ -type d 
        COMMAND grep -vE ${GREP_TEXT} # 过滤目录 U
        OUTPUT_VARIABLE dirs_list
        )
    string(REPLACE "\n" ";" dirs_list ${dirs_list})
    # 添加所有源文件与头文件
    foreach(item ${dirs_list})
        target_include_directories(${MODULE_NAME} PUBLIC ${item})
        aux_source_directory(${item} MODULE_SOURCES)
    endforeach() 
else() 
    message("==========> [${PROJECT_NAME}]-[${MODULE_NAME}] 不使用本地AI分析, 不设置AI的源文件和头文件 <==========")
endif()

# 设置的源文件
target_sources(${MODULE_NAME} PRIVATE ${MODULE_SOURCES})

# 链接库
target_link_libraries(${MODULE_NAME} PUBLIC ${LIBS} ${PLATFORM_LIBS} aiInferenceModel_CPU)

# 打印消息，指示模块设置成功
message("==========> [${PROJECT_NAME}] 使用[${MODULE_NAME}]模块 <==========")   

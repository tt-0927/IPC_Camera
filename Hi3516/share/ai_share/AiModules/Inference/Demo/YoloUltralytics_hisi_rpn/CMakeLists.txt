# 版本限定
cmake_minimum_required(VERSION 3.12)

# 设置程序名称
project("YoloUltralytics_HiSiDemo")

# NPU_ARM
set(CROSS_PREFIX "arm-v01c02-linux-musleabi-")

# 编译链设置
string(STRIP "${CROSS_PREFIX}" CROSS_PREFIX)
# 根据前缀自动设置编译工具
set(CMAKE_C_COMPILER    "${CROSS_PREFIX}gcc")
set(CMAKE_CXX_COMPILER  "${CROSS_PREFIX}g++")
set(CMAKE_AR            "${CROSS_PREFIX}ar")
set(CMAKE_RANLIB        "${CROSS_PREFIX}ranlib")
set(CMAKE_LINKER        "${CROSS_PREFIX}ld")

# 推理依赖
include(${CMAKE_CURRENT_LIST_DIR}/../../../../../ai_model/hisilicon_arm_musleabi/ai/ai.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../../../../ai_model/hisilicon_arm_musleabi/opencv/opencv.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../Hisilicon/TargetDetection/YoloUltralytics_rpn/YoloUltralytics_rpn.cmake)

# 文件  
set (SRC_PATH
    # 后处理依赖
    ${CMAKE_CURRENT_LIST_DIR}/../../Common/json
)
foreach(item ${SRC_PATH})
    target_include_directories(${INFERENCE_NAME} INTERFACE ${item})
    aux_source_directory(${item} MODULE_SOURCES)
endforeach()

# 设置生成文件存放路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../output/bin)

# 编译
add_executable(${PROJECT_NAME} ${MODULE_SOURCES} main.cpp)

# 链接库路径
target_link_libraries(${PROJECT_NAME}
    YoloUltralytics_rpn
    libopencv
)



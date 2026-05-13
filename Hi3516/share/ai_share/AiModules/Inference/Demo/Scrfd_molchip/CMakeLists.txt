# 版本限定
cmake_minimum_required(VERSION 3.12)

# 设置程序名称
project("Scrfd_RKDemo")

include(${CMAKE_CURRENT_LIST_DIR}/../../../../../platform/platform.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../../../../platform/lib/rockit/rockit.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../RK/KeyPoint/Scrfd/Scrfd.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../../../../platform/lib/opencv/opencv.cmake)

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
    Scrfd_RK
    libopencv
)



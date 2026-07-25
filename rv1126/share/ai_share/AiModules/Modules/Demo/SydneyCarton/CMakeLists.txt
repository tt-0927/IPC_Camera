# 版本限定
cmake_minimum_required(VERSION 3.12)

# 设置程序名称
project("SydneyCarton_Demo")

include(${CMAKE_CURRENT_LIST_DIR}/../../../../../platform/platform.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../../../../platform/lib/opencv/opencv.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../SydneyCarton/SydneyCarton.cmake)

# 设置生成文件存放路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../output/bin)

# 编译
add_executable(${PROJECT_NAME} main.cpp)

# 链接库路径
target_link_libraries(${PROJECT_NAME}
    SydneyCarton_Module
)



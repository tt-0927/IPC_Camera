cmake_minimum_required(VERSION 3.5)
project(ShapeFactory)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)

# Find OpenCV package
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# 添加源文件和头文件路径
include_directories(include)
file(GLOB SOURCES "src/*.cpp")

# 模型读取
add_subdirectory(onnx-model)


# 生成可执行文件
add_executable(ShapeFactory ${SOURCES})

target_link_libraries(ShapeFactory PRIVATE ${OpenCV_LIBS} onnx_model)

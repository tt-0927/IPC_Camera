
# 查找 OpenCV 库
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# 查找 ONNX Runtime 库
set(ONNXRUNTIME_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/onnxruntime-linux-x64-1.10.0/include)
set(ONNXRUNTIME_LIBRARY ${CMAKE_CURRENT_LIST_DIR}/onnxruntime-linux-x64-1.10.0/lib/libonnxruntime.so)
include_directories(${ONNXRUNTIME_INCLUDE_DIR})

# 添加可执行文件
add_library(onnx_model Model.cpp)

# 链接 OpenCV 库
target_link_libraries(onnx_model ${OpenCV_LIBS} ${ONNXRUNTIME_LIBRARY})


# 包含头文件目录
target_include_directories(onnx_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${ONNXRUNTIME_INCLUDE_DIR})

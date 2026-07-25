# opencv.cmake

# 使用相对路径获取 的包含和库路径
set(OPENCV_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(OPENCV_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(OPENCV_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/opencv4")

link_directories(${OPENCVFONT_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB OPENCV_LIBRARY 
	"${OPENCV_LIBRARY_DIR}/lib*.so*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libopencv INTERFACE)
target_link_libraries(libopencv INTERFACE ${OPENCV_LIBRARY})
target_include_directories(libopencv INTERFACE ${OPENCV_INCLUDE_DIR})

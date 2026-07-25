# x264.cmake

# 使用相对路径获取 的包含和库路径
set(X264_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(X264_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${X264_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB X264_LIBRARY 
    "${X264_LIBRARY_DIR}/lib*.so*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libx264 INTERFACE)
target_link_libraries(libx264 INTERFACE ${X264_LIBRARY})
target_include_directories(libx264 INTERFACE ${X264_INCLUDE_DIR})

# x265.cmake

# 使用相对路径获取 的包含和库路径
set(X265_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(X265_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${X265_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB X265_LIBRARY 
    "${X265_LIBRARY_DIR}/lib*.so*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libx265 INTERFACE)
target_link_libraries(libx265 INTERFACE ${X265_LIBRARY})
target_include_directories(libx265 INTERFACE ${X265_INCLUDE_DIR})

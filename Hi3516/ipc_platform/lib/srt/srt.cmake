# srt.cmake

# 使用相对路径获取 的包含和库路径
set(SRT_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(SRT_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${SRT_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB SRT_LIBRARY 
    "${SRT_LIBRARY_DIR}/lib*.so*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libsrt INTERFACE)
target_link_libraries(libsrt INTERFACE ${SRT_LIBRARY})
target_include_directories(libsrt INTERFACE ${SRT_INCLUDE_DIR})

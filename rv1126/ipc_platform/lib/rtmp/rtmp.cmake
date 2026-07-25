# rtmp.cmake

# 使用相对路径获取 的包含和库路径
set(RTMP_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(RTMP_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${RTMP_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB RTMP_LIBRARY 
    "${RTMP_LIBRARY_DIR}/lib*.so*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(librtmp INTERFACE)
target_link_libraries(librtmp INTERFACE ${RTMP_LIBRARY})
target_include_directories(librtmp INTERFACE ${RTMP_INCLUDE_DIR})

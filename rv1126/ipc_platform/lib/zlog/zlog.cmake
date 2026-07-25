# zlog.cmake

# 使用相对路径获取 的包含和库路径
set(ZLOG_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(ZLOG_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${ZLOG_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB ZLOG_LIBRARY 
    "${ZLOG_LIBRARY_DIR}/lib*.so*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libzlog INTERFACE)
target_link_libraries(libzlog INTERFACE ${ZLOG_LIBRARY})
target_include_directories(libzlog INTERFACE ${ZLOG_INCLUDE_DIR})

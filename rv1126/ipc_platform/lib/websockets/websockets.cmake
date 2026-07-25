# websockets.cmake

# 使用相对路径获取 的包含和库路径
set(WEBSOCKETS_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(WEBSOCKETS_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(WEBSOCKETS_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/libwebsockets")
set(WEBSOCKETS_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/libwebsockets/abstract")
set(WEBSOCKETS_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/libwebsockets/abstract/protocols")
set(WEBSOCKETS_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/libwebsockets/abstract/transports")

link_directories(${WEBSOCKETS_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB WEBSOCKETS_LIBRARY
    "${WEBSOCKETS_LIBRARY_DIR}/lib*.so*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libwebsockets INTERFACE)
target_link_libraries(libwebsockets INTERFACE ${WEBSOCKETS_LIBRARY})
target_include_directories(libwebsockets INTERFACE ${WEBSOCKETS_INCLUDE_DIR})

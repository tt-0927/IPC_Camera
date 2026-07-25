# upnp.cmake

# 使用相对路径获取 的包含和库路径
set(UPNP_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(UPNP_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(UPNP_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/upnp")

link_directories(${UPNP_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB UPNP_LIBRARY 
    "${UPNP_LIBRARY_DIR}/lib*.so*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libupnp INTERFACE)
target_link_libraries(libupnp INTERFACE ${UPNP_LIBRARY})
target_include_directories(libupnp INTERFACE ${UPNP_INCLUDE_DIR})

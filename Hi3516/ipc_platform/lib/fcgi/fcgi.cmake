# fcgi.cmake

# 使用相对路径获取 的包含和库路径
set(FCGI_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(FCGI_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${FCGI_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB FCGI_LIBRARY
    "${FCGI_LIBRARY_DIR}/lib*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libfcgi INTERFACE)
target_link_libraries(libfcgi INTERFACE ${FCGI_LIBRARY})
target_include_directories(libfcgi INTERFACE ${FCGI_INCLUDE_DIR})

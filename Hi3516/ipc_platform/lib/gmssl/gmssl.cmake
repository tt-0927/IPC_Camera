# gmssl.cmake

# 使用相对路径获取 的包含和库路径
set(GMSSL_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(GMSSL_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${GMSSL_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB GMSSL_LIBRARY 
    "${GMSSL_LIBRARY_DIR}/lib*.so*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libgmssl INTERFACE)
target_link_libraries(libgmssl INTERFACE ${GMSSL_LIBRARY})
target_include_directories(libgmssl INTERFACE ${GMSSL_INCLUDE_DIR})

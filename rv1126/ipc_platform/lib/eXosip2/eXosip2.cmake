
# 库路径
set(EXOSIP2_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(EXOSIP2_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${EXOSIP2_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB EXOSIP2_LIBRARY
    "${EXOSIP2_LIBRARY_DIR}/lib*.so*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libeXosip2 INTERFACE)
target_link_libraries(libeXosip2 INTERFACE ${EXOSIP2_LIBRARY})
target_include_directories(libeXosip2 INTERFACE ${EXOSIP2_INCLUDE_DIR})



# twolame.cmake

# 使用相对路径获取 的包含和库路径
set(TWOLAME_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(TWOLAME_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${TWOLAME_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB TWOLAME_LIBRARY 
    "${TWOLAME_LIBRARY_DIR}/lib*.so*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libtwolame INTERFACE)
target_link_libraries(libtwolame INTERFACE ${TWOLAME_LIBRARY})
target_include_directories(libtwolame INTERFACE ${TWOLAME_INCLUDE_DIR})

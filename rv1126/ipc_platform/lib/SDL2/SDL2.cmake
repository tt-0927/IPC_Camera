# SDL2.cmake

# 使用相对路径获取 的包含和库路径
set(SDL2_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(SDL2_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(SDL2_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/SDL2")

link_directories(${SDL2_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB SDL2_LIBRARY 
    "${SDL2_LIBRARY_DIR}/lib*.so*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libSDL2 INTERFACE)
target_link_libraries(libSDL2 INTERFACE ${SDL2_LIBRARY})
target_include_directories(libSDL2 INTERFACE ${SDL2_INCLUDE_DIR})

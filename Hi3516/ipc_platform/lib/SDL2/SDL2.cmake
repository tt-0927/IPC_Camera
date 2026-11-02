# SDL2.cmake

include("${CMAKE_CURRENT_LIST_DIR}/../static_first.cmake")

set(SDL2_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(SDL2_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(SDL2_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/SDL2")

link_directories(${SDL2_LIBRARY_DIR})

# 旧逻辑备份（纯动态链接）。回退步骤：注释掉下面 ipc_platform_static_first 调用，
# 解开以下三行
# file(GLOB SDL2_LIBRARY
#     "${SDL2_LIBRARY_DIR}/lib*.so*"
# )

# 静态优先选库，公共逻辑见 ../static_first.cmake
ipc_platform_static_first(SDL2_LIBRARY "${SDL2_LIBRARY_DIR}")

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libSDL2 INTERFACE)
target_link_libraries(libSDL2 INTERFACE ${SDL2_LIBRARY})
target_include_directories(libSDL2 INTERFACE ${SDL2_INCLUDE_DIR})

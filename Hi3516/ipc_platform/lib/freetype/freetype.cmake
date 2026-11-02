# freetype.cmake

include("${CMAKE_CURRENT_LIST_DIR}/../static_first.cmake")

set(FREETYPE_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(FREETYPE_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/freetype2"
"${CMAKE_CURRENT_LIST_DIR}/include/freetype2/freetype")

link_directories(${FREETYPE_LIBRARY_DIR})

# 旧逻辑备份（纯动态链接）。回退步骤：注释掉下面 ipc_platform_static_first 调用，
# 解开以下三行
# file(GLOB FREETYPE_LIBRARY
#     "${FREETYPE_LIBRARY_DIR}/lib*.so*"
# )

# 静态优先选库，公共逻辑见 ../static_first.cmake
ipc_platform_static_first(FREETYPE_LIBRARY "${FREETYPE_LIBRARY_DIR}")

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libfreetype INTERFACE)
target_link_libraries(libfreetype INTERFACE ${FREETYPE_LIBRARY})
target_include_directories(libfreetype INTERFACE ${FREETYPE_INCLUDE_DIR})

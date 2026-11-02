# ffmpeg.cmake

include("${CMAKE_CURRENT_LIST_DIR}/../static_first.cmake")

set(FFMPEG_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(FFMPEG_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${FFMPEG_LIBRARY_DIR})

# 旧逻辑备份（纯动态链接）。回退步骤：注释掉下面 ipc_platform_static_first 调用，
# 解开以下三行
# file(GLOB FFMPEG_LIBRARY
#     "${FFMPEG_LIBRARY_DIR}/lib*.so*"
# )

# 静态优先选库，公共逻辑见 ../static_first.cmake
ipc_platform_static_first(FFMPEG_LIBRARY "${FFMPEG_LIBRARY_DIR}")

# 创建 libffmpeg 接口库，将所有子库组合
add_library(libffmpeg INTERFACE)
target_link_libraries(libffmpeg INTERFACE ${FFMPEG_LIBRARY})
target_include_directories(libffmpeg INTERFACE ${FFMPEG_INCLUDE_DIR})

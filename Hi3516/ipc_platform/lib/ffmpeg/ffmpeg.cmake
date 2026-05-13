# ffmpeg.cmake

set(FFMPEG_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(FFMPEG_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${FFMPEG_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB FFMPEG_LIBRARY 
    "${FFMPEG_LIBRARY_DIR}/lib*.so*"
)

# 创建 libffmpeg 接口库，将所有子库组合
add_library(libffmpeg INTERFACE)
target_link_libraries(libffmpeg INTERFACE ${FFMPEG_LIBRARY})
target_include_directories(libffmpeg INTERFACE ${FFMPEG_INCLUDE_DIR})

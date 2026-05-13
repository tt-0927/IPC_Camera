# ZLMediaKit.cmake

# 使用相对路径获取 的包含和库路径
set(ZLMEDIAKIT_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(ZLMEDIAKIT_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${ZLMEDIAKIT_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB ZLMEDIAKIT_LIBRARY 
    "${ZLMEDIAKIT_LIBRARY_DIR}/lib*.so*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libmk_api INTERFACE)
target_link_libraries(libmk_api INTERFACE ${ZLMEDIAKIT_LIBRARY})
target_include_directories(libmk_api INTERFACE ${ZLMEDIAKIT_INCLUDE_DIR})

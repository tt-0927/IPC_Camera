# fdk-aac.cmake

# 使用相对路径获取 的包含和库路径
set(FDK-AAC_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(FDK-AAC_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${FDK-AAC_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB FDK-AAC_LIBRARY 
    "${FDK-AAC_LIBRARY_DIR}/lib*.so*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libfdk-aac INTERFACE)
target_link_libraries(libfdk-aac INTERFACE ${FDK-AAC_LIBRARY})
target_include_directories(libfdk-aac INTERFACE ${FDK-AAC_INCLUDE_DIR})

# NetTVSDK.cmake

# 使用相对路径获取 的包含和库路径
set(NetTVSDK_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")

link_directories(${NetTVSDK_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB NetTVSDK_LIBRARY 
    "${NetTVSDK_LIBRARY_DIR}/lib*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libNetTVSDK INTERFACE)
target_link_libraries(libNetTVSDK INTERFACE ${NetTVSDK_LIBRARY})
# target_include_directories(libNetTVSDK INTERFACE ${NetTVSDK_INCLUDE_DIR})

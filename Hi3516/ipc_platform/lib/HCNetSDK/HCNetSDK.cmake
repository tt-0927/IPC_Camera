# HCNetSDK.cmake

# 使用相对路径获取 的包含和库路径
set(HCNetSDK_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")

link_directories(${HCNetSDK_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB HCNetSDK_LIBRARY 
    "${HCNetSDK_LIBRARY_DIR}/lib*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libHCNetSDK INTERFACE)
target_link_libraries(libHCNetSDK INTERFACE ${HCNetSDK_LIBRARY})
# target_include_directories(libHCNetSDK INTERFACE ${HCNetSDK_INCLUDE_DIR})

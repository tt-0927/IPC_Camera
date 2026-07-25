
# rockit-config.cmake

# 使用相对路径获取 rockit 的包含和库路径
set(ROCKIT_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")

link_directories(${ROCKIT_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB ROCKIT_LIBRARY "${ROCKIT_LIBRARY_DIR}/lib*")

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library( librknnrt INTERFACE)
target_link_libraries( librknnrt INTERFACE ${ROCKIT_LIBRARY})

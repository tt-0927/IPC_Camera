# MemCheck.cmake

# 使用相对路径获取 的包含和库路径
set(MEMCHECK_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")

link_directories(${MEMCHECK_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB MEMCHECK_LIBRARY 
    "${MEMCHECK_LIBRARY_DIR}/lib*.so*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libMemoryCheck INTERFACE)
target_link_libraries(libMemoryCheck INTERFACE ${MEMCHECK_LIBRARY})

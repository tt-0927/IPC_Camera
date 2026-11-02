# MemCheck.cmake

include("${CMAKE_CURRENT_LIST_DIR}/../static_first.cmake")

set(MEMCHECK_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")

link_directories(${MEMCHECK_LIBRARY_DIR})

# 旧逻辑备份（纯动态链接）。回退步骤：注释掉下面 ipc_platform_static_first 调用，
# 解开以下三行
# file(GLOB MEMCHECK_LIBRARY
#     "${MEMCHECK_LIBRARY_DIR}/lib*.so*"
# )

# 静态优先选库，公共逻辑见 ../static_first.cmake
ipc_platform_static_first(MEMCHECK_LIBRARY "${MEMCHECK_LIBRARY_DIR}")

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libMemoryCheck INTERFACE)
target_link_libraries(libMemoryCheck INTERFACE ${MEMCHECK_LIBRARY})

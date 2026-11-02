# ss_mpi.cmake

include("${CMAKE_CURRENT_LIST_DIR}/../static_first.cmake")

# 使用相对路径获取 的包含和库路径
set(SS_MPI_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")

link_directories(${SS_MPI_LIBRARY_DIR})

# 旧逻辑备份（纯动态链接）。回退步骤：注释掉下面 ipc_platform_static_first 调用，
# 解开以下三行
# file(GLOB SS_MPI_LIBRARY 
#     "${SS_MPI_LIBRARY_DIR}/lib*"
# )

# 静态优先选库，公共逻辑见 ../static_first.cmake
ipc_platform_static_first(SS_MPI_LIBRARY "${SS_MPI_LIBRARY_DIR}")

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libss_mpi INTERFACE)
target_link_libraries(libss_mpi INTERFACE ${SS_MPI_LIBRARY})
target_include_directories(libss_mpi INTERFACE ${SS_MPI_INCLUDE_DIR})

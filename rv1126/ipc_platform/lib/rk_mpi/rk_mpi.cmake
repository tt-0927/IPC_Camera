# rk_mpi.cmake

# 使用相对路径获取 的包含和库路径
set(RK_MPI_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")

link_directories(${RK_MPI_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB RK_MPI_LIBRARY 
    "${RK_MPI_LIBRARY_DIR}/lib*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(librk_mpi INTERFACE)
target_link_libraries(librk_mpi INTERFACE ${RK_MPI_LIBRARY})
target_include_directories(librk_mpi INTERFACE ${RK_MPI_INCLUDE_DIR})

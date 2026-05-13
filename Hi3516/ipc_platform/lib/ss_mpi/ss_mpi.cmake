# ss_mpi.cmake

# 使用相对路径获取 的包含和库路径
set(SS_MPI_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")

link_directories(${SS_MPI_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB SS_MPI_LIBRARY 
    "${SS_MPI_LIBRARY_DIR}/lib*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libss_mpi INTERFACE)
target_link_libraries(libss_mpi INTERFACE ${SS_MPI_LIBRARY})
target_include_directories(libss_mpi INTERFACE ${SS_MPI_INCLUDE_DIR})

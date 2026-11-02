# z.cmake

include("${CMAKE_CURRENT_LIST_DIR}/../static_first.cmake")

set(Z_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(Z_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${Z_LIBRARY_DIR})

# 旧逻辑备份（纯动态链接）。回退步骤：注释掉下面 ipc_platform_static_first 调用，
# 解开以下三行
# file(GLOB Z_LIBRARY
#     "${Z_LIBRARY_DIR}/lib*.so*"
# )

# 静态优先选库，公共逻辑见 ../static_first.cmake
ipc_platform_static_first(Z_LIBRARY "${Z_LIBRARY_DIR}")

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libz INTERFACE)
target_link_libraries(libz INTERFACE ${Z_LIBRARY})
target_include_directories(libz INTERFACE ${Z_INCLUDE_DIR})
# libz 静态化后，libcurl.so 等动态库的 libz.so.1 传递依赖在链接期找不到实体
# （本工具链 ld 解析传递依赖时不搜索 -rpath，只认 -rpath-link），补上指向本目录。
# 仅影响链接期，不进产物运行时搜索路径：板端由根文件系统 /lib/libz.so.1 满足
target_link_options(libz INTERFACE "-Wl,-rpath-link,${Z_LIBRARY_DIR}")


# 库路径
include("${CMAKE_CURRENT_LIST_DIR}/../static_first.cmake")

set(OSIP2_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(OSIP2_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${OSIP2_LIBRARY_DIR})

# 旧逻辑备份（纯动态链接）。回退步骤：注释掉下面 ipc_platform_static_first 调用，
# 解开以下三行
# file(GLOB OSIP2_LIBRARY
#     "${OSIP2_LIBRARY_DIR}/lib*.so*"
# )

# 静态优先选库，公共逻辑见 ../static_first.cmake
ipc_platform_static_first(OSIP2_LIBRARY "${OSIP2_LIBRARY_DIR}")

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libosip2 INTERFACE)
target_link_libraries(libosip2 INTERFACE ${OSIP2_LIBRARY})
target_include_directories(libosip2 INTERFACE ${OSIP2_INCLUDE_DIR})
# 注意：libeXosip2.a 引用 580 个 osip_* 符号，libosip2 必须排在 libeXosip2
# 之后链接，顺序由消费方的 target_link_libraries 负责


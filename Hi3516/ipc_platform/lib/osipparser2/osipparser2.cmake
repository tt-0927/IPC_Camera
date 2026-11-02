# 库路径
include("${CMAKE_CURRENT_LIST_DIR}/../static_first.cmake")

set(OSIPPARSER2_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(OSIPPARSER2_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${OSIPPARSER2_LIBRARY_DIR})

# 旧逻辑备份（纯动态链接）。回退步骤：注释掉下面 ipc_platform_static_first 调用，
# 解开以下三行
# file(GLOB OSIPPARSER2_LIBRARY
#     "${OSIPPARSER2_LIBRARY_DIR}/lib*.so*"
# )

# 静态优先选库，公共逻辑见 ../static_first.cmake
ipc_platform_static_first(OSIPPARSER2_LIBRARY "${OSIPPARSER2_LIBRARY_DIR}")

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libosipparser2 INTERFACE)
target_link_libraries(libosipparser2 INTERFACE ${OSIPPARSER2_LIBRARY})
target_include_directories(libosipparser2 INTERFACE ${OSIPPARSER2_INCLUDE_DIR})
# 同 osip2.cmake：libosipparser2 必须排在 libeXosip2 之后链接，
# 顺序由消费方的 target_link_libraries 负责



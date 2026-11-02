# snmp++.cmake

include("${CMAKE_CURRENT_LIST_DIR}/../static_first.cmake")

set(SNMP_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(SNMP_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${SNMP_LIBRARY_DIR})

# 旧逻辑备份（纯动态链接）。回退步骤：注释掉下面 ipc_platform_static_first 调用，
# 解开以下三行
# file(GLOB SNMP_LIBRARY
#     "${SNMP_LIBRARY_DIR}/lib*.so*"
# )

# 静态优先选库，公共逻辑见 ../static_first.cmake
ipc_platform_static_first(SNMP_LIBRARY "${SNMP_LIBRARY_DIR}")

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libsnmp++ INTERFACE)
target_link_libraries(libsnmp++ INTERFACE ${SNMP_LIBRARY})
target_include_directories(libsnmp++ INTERFACE ${SNMP_INCLUDE_DIR})
# 注意：libsnmp++.a 引用 22 个 OpenSSL 符号，libagent++.a 依赖 Snmp_pp 命名
# 空间。链接顺序要求：libagent++ 在 libsnmp++ 之前、openssl 在两者之后，
# 由消费方的 target_link_libraries 负责

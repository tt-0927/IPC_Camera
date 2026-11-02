# xml2.cmake

include("${CMAKE_CURRENT_LIST_DIR}/../static_first.cmake")

set(XML2_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(XML2_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${XML2_LIBRARY_DIR})

# 旧逻辑备份（纯动态链接）。回退步骤：注释掉下面 ipc_platform_static_first 调用，
# 解开以下三行
# file(GLOB XML2_LIBRARY
#     "${XML2_LIBRARY_DIR}/lib*.so*"
# )

# 静态优先选库，公共逻辑见 ../static_first.cmake
ipc_platform_static_first(XML2_LIBRARY "${XML2_LIBRARY_DIR}")

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libxml2 INTERFACE)
target_link_libraries(libxml2 INTERFACE ${XML2_LIBRARY})
target_include_directories(libxml2 INTERFACE ${XML2_INCLUDE_DIR})
# 注意：libxml2.a 引用 zlib 符号（crc32/inflate/deflate）。工具链默认
# --as-needed，libz 必须排在 libxml2 之后（单遍扫描才能捞到归档成员），
# 该顺序由消费方的 target_link_libraries 负责，见 main_app/CMakeLists.txt

# iconv.cmake

include("${CMAKE_CURRENT_LIST_DIR}/../static_first.cmake")

set(ICONV_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(ICONV_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

# 注意：这里禁止加 link_directories(${ICONV_LIBRARY_DIR})。交叉工具链会隐式
# 追加 -liconv（musl 静态 iconv 实现），一旦本目录进入 -L 搜索路径，隐式
# -liconv 会被本目录里只导出改名符号（libiconv_open 等）的 libiconv.so 截胡，
# xml2.a/SDL2.a 里引用的 iconv_open 将永远解析不到，链接直接失败。


# 旧逻辑备份（纯动态链接）。回退步骤：注释掉下面 ipc_platform_static_first 调用，
# 解开以下三行
# file(GLOB ICONV_LIBRARY
#     "${ICONV_LIBRARY_DIR}/lib*.so*"
# )

# 静态优先选库，公共逻辑见 ../static_first.cmake
ipc_platform_static_first(ICONV_LIBRARY "${ICONV_LIBRARY_DIR}")

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libiconv INTERFACE)
target_link_libraries(libiconv INTERFACE ${ICONV_LIBRARY})
target_include_directories(libiconv INTERFACE ${ICONV_INCLUDE_DIR})

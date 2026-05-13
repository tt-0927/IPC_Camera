# 库路径
set(OSIPPARSER2_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(OSIPPARSER2_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${OSIPPARSER2_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB OSIPPARSER2_LIBRARY
    "${OSIPPARSER2_LIBRARY_DIR}/lib*.so*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libosipparser2 INTERFACE)
target_link_libraries(libosipparser2 INTERFACE ${OSIPPARSER2_LIBRARY})
target_include_directories(libosipparser2 INTERFACE ${OSIPPARSER2_INCLUDE_DIR})



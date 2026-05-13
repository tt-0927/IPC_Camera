# sqlite3.cmake

# 使用相对路径获取 的包含和库路径
set(SQLITE3_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(SQLITE3_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${SQLITE3_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB SQLITE3_LIBRARY 
    "${SQLITE3_LIBRARY_DIR}/lib*.so*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libsqlite3 INTERFACE)
target_link_libraries(libsqlite3 INTERFACE ${SQLITE3_LIBRARY})
target_include_directories(libsqlite3 INTERFACE ${SQLITE3_INCLUDE_DIR})

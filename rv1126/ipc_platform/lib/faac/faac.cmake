# faac.cmake

# 使用相对路径获取 的包含和库路径
set(FAAC_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(FAAC_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${FAAC_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB FAAC_LIBRARY 
    "${FAAC_LIBRARY_DIR}/lib*.so*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libfaac INTERFACE)
target_link_libraries(libfaac INTERFACE ${FAAC_LIBRARY})
target_include_directories(libfaac INTERFACE ${FAAC_INCLUDE_DIR})

# agent++.cmake

# 使用相对路径获取 的包含和库路径
set(AGENT_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(AGENT_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${AGENT_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB AGENT_LIBRARY 
    "${AGENT_LIBRARY_DIR}/lib*.so*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libagent++ INTERFACE)
target_link_libraries(libagent++ INTERFACE ${AGENT_LIBRARY})
target_include_directories(libagent++ INTERFACE ${AGENT_INCLUDE_DIR})

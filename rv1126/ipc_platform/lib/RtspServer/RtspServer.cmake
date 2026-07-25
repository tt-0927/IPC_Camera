# RtspServer.cmake

# 使用相对路径获取 的包含和库路径
set(RTSPSERVER_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(RTSPSERVER_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(RTSPSERVER_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/mediaServer")

link_directories(${RTSPSERVER_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB RTSPSERVER_LIBRARY 
	"${RTSPSERVER_LIBRARY_DIR}/lib*.so*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libRtspServer INTERFACE)
target_link_libraries(libRtspServer INTERFACE ${RTSPSERVER_LIBRARY})
target_include_directories(libRtspServer INTERFACE ${RTSPSERVER_INCLUDE_DIR})

# xml2.cmake

# 使用相对路径获取 的包含和库路径
set(XML2_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(XML2_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${XML2_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB XML2_LIBRARY 
	"${XML2_LIBRARY_DIR}/lib*.so*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libxml2 INTERFACE)
target_link_libraries(libxml2 INTERFACE ${XML2_LIBRARY})
target_include_directories(libxml2 INTERFACE ${XML2_INCLUDE_DIR})

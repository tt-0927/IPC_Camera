
# 库路径
set(ONVIF_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(ONVIF_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${ONVIF_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB ONVIF_LIBRARY
    "${ONVIF_LIBRARY_DIR}/lib*.so*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libonvif INTERFACE)
target_link_libraries(libonvif INTERFACE ${ONVIF_LIBRARY})
target_include_directories(libonvif INTERFACE ${ONVIF_INCLUDE_DIR})



# common 模块 CMake 配置
# 线程安全的帧队列等公共组件

#头文件
set (COMMON_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${COMMON_INCLUDE})
    include_directories ( ${item} ) 
endforeach()

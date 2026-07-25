# gb35114.cmake

# 引用
include(${CMAKE_CURRENT_LIST_DIR}/core/core.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/openssl/openssl.cmake)
# deprecated: gmssl 目录已从编译中排除，文件保留作历史参考，勿删除
# include(${CMAKE_CURRENT_LIST_DIR}/gmssl/gmssl.cmake)

# 头文件
set (INCLUDE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)

# 源文件
set (SOURCE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)

# 添加头文件
foreach(item ${INCLUDE_PATH})
    include_directories (${item}) 
endforeach()

# 添加源文件
foreach(item ${SOURCE_PATH})
    aux_source_directory (${item} SOURCE_LIST)
endforeach()

list(APPEND SRC_LIST ${SOURCE_LIST})

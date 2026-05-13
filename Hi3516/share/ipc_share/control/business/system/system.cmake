# system.cmake

# 引用
include(${CMAKE_CURRENT_LIST_DIR}/time/time.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/user/user.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/register/register.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/ip_filter/ip_filter.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/ssh/ssh.cmake)
# include(${CMAKE_CURRENT_LIST_DIR}/gpioCtrl/gpioCtrl.cmake)

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

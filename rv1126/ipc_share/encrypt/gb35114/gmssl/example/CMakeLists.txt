# 版本限定
cmake_minimum_required(VERSION 3.16.2)

# 程序名称
SET(PRO_NAME "gmssl_example")
project(${PRO_NAME})

# 头文件
set (INCLUDE_PATH
    # main
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${SRC_ROOT_PATH}/main_app/common/define
)

# 源文件
set (SOURCE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# 库文件
set (LIBS

)

# 添加头文件
foreach(item ${INCLUDE_PATH})
    include_directories (${item})
endforeach()

# 添加源文件
foreach(item ${SOURCE_PATH})
    aux_source_directory (${item} SOURCE_LIST)
endforeach()

# 编译
add_executable (${PRO_NAME} ${SOURCE_LIST})

# 链接库
target_link_libraries (${PRO_NAME} ${LIBS})

message("==========> Generate " ${PRO_NAME} " Makefile Success <==========")

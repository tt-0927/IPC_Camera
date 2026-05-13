#版本限定
cmake_minimum_required(VERSION 3.10.2)

# 设置程序名称
project("network")
set(C_COMPILER "gcc")
set(CXX_COMPILER "g++")
#设置父目录给的编译链
set(CMAKE_C_COMPILER ${C_COMPILER})
set(CMAKE_CXX_COMPILER ${CXX_COMPILER})

set(CMAKE_C_FLAGS " -g")
set(CMAKE_CXX_FLAGS " -g")


include(${CMAKE_CURRENT_LIST_DIR}/../tcp/tcp.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../uds/uds.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../websocket/websocket.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../data_format/json/json.cmake)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/)
aux_source_directory (${CMAKE_CURRENT_SOURCE_DIR}/ PROJECT_SRC_LIST)
list(APPEND SRC_LIST ${PROJECT_SRC_LIST} )
#添加库
set (LIBS -lpthread m rt -lstdc++)
set (LOCAL_LIBS
 -lwebsockets
 )
 #链接库路径
 link_directories(/home/zjc/workdir/library/output)


# 打印找到的文件
foreach(source_file ${SRC_LIST})
    message(STATUS "Source file: ${source_file}")
endforeach()
#编译
add_executable (${PROJECT_NAME} ${SRC_LIST}  )
#链接库路径
target_link_libraries (${PROJECT_NAME} ${LIBS} ${LOCAL_LIBS} ${EXTERNAL_LIBS})




message("==========> 生成 [" ${PROJECT_NAME} "] Makefile文件 成功 <==========")

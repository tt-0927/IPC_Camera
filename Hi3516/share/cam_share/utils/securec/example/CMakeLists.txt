#版本限定
cmake_minimum_required(VERSION 3.10.2)

# 设置程序名称
project("securec")

SET(CMAKE_C_COMPILER "aarch64-molv1-linux-gnu-gcc")
SET(CMAKE_CXX_COMPILER "aarch64-molv1-linux-gnu-g++")

set(CMAKE_C_FLAGS "-Wall -g")
set(CMAKE_CXX_FLAGS "-Wall -std=c++17 -g")


include(${CMAKE_CURRENT_LIST_DIR}/../securec.cmake)

#添加库
set (LIBS -lpthread  m -lstdc++)
#链接库路径
link_directories()

#设置生成文件存放路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../output/bin)
message("设置生成文件存放路径:" ${CMAKE_CURRENT_LIST_DIR}/output/../bin)


add_subdirectory(memcpy_s)
add_subdirectory(wmemcpy_s)
add_subdirectory(memmove_s)
add_subdirectory(wmemmove_s)
add_subdirectory(memset_s)
add_subdirectory(gets_s)
add_subdirectory(strcat_s)
add_subdirectory(wcscat_s)
add_subdirectory(strncat_s)
add_subdirectory(strcpy_s)
add_subdirectory(strtok_s)
#add_subdirectory(vscanf_s)
add_subdirectory(vsprintf_s)
add_subdirectory(sprintf_s)
add_subdirectory(vsnprintf_s)
add_subdirectory(snprintf_s)

message("==========> 生成 [" ${PROJECT_NAME} "] Makefile文件 成功 <==========")


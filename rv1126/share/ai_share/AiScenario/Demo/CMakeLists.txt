# 版本限定
cmake_minimum_required(VERSION 3.10.2)

# 设置程序名称
project("AiScenarioDemo")

# 设置C++标准为17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
SET(CMAKE_C_COMPILER "aarch64-buildroot-linux-gnu-gcc")
SET(CMAKE_CXX_COMPILER "aarch64-buildroot-linux-gnu-g++")
SET(CMAKE_AR "aarch64-buildroot-linux-gnu-ar")
SET(CMAKE_LINKER "aarch64-buildroot-linux-gnu-ld")

#设置生成文件存放路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../output/bin)
message("设置生成文件存放路径:" ${CMAKE_CURRENT_LIST_DIR}/../output/bin)


set(PROJECT_LIB_PATH ${CMAKE_CURRENT_LIST_DIR}/../../lib/rk3588/)
message("==========> 当前编译库路径: ${PROJECT_LIB_PATH}")
link_directories(${PROJECT_LIB_PATH} | ${PROJECT_LIB_PATH}/staticso)

# 检查 PLATFORM 变量是否已定义，如果未定义则设置默认值
if (NOT DEFINED PLATFORM)
    set(PLATFORM "RK")
endif()


# 添加调试符号
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g ")

# 文件
set (SRC_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    
    ${CMAKE_CURRENT_LIST_DIR}/../../../bl_share/media/image/
    ${CMAKE_CURRENT_LIST_DIR}/../../../bl_share/log/nslog/
    ${CMAKE_CURRENT_LIST_DIR}/../../../bl_share/network/common/
)

foreach(item ${SRC_PATH})
    include_directories(${item}) 
    aux_source_directory(${item} SRC_LIST)
endforeach()

# 添加模块
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../AiScenario AiScenarioBuild)

# 编译
add_executable (${PROJECT_NAME} ${SRC_LIST}) 

# 链接库路径
target_link_libraries(${PROJECT_NAME} AI_SCENARIO)

message("==========> 生成 [" ${PROJECT_NAME} "] Makefile文件 成功 <==========")

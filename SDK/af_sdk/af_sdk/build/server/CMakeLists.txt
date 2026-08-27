# 最低CMake版本要求（适配嵌入式交叉编译环境）
cmake_minimum_required(VERSION 3.10)
set(LIB_NAME "NetTVSDKServer") 
# 库的名称 
project(${LIB_NAME})

#  指定C++标准（和工具链一致，C++11）
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # 禁用编译器扩展，保证跨平台

# 链接时省略符号信息
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s -Os -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s -Os -ffunction-sections -fdata-sections")

# 控制 .so 和 .elf 链接行为 链接时清除未被引用的函数/变量
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections")

# 默认隐藏所有符号 (对应 Linux -fvisibility=hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
# 隐藏内联函数的符号 (防止头文件中的实现泄露)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

set(SDK_BUILD_PATH  ${CMAKE_CURRENT_LIST_DIR})
set(SDK_SERVER_PATH ${CMAKE_CURRENT_LIST_DIR}/../../sdk_server)
set(SDK_SHARE_PATH  ${CMAKE_CURRENT_LIST_DIR}/../../sdk_share)

file(GLOB_RECURSE LIB_SRC 
    "${SDK_SERVER_PATH}/src/*.c"
    "${SDK_SERVER_PATH}/src/*.cpp"
    "${SDK_SHARE_PATH}/*.c"
    "${SDK_SHARE_PATH}/*.cpp"
)

file(GLOB_RECURSE LIB_INCLUDE_TMP
    LIST_DIRECTORIES true  
    "${SDK_SERVER_PATH}/*" 
    "${SDK_SHARE_PATH}/*"      
)

set(LIB_INCLUDE "")  
foreach(path ${LIB_INCLUDE_TMP})
    # 判断当前路径是否是目录
    if(IS_DIRECTORY ${path})
        list(APPEND LIB_INCLUDE ${path})
    endif()
endforeach()

# 添加静态库
add_library(${LIB_NAME}-static STATIC ${LIB_SRC})
# 设置静态库属性 
set_target_properties(${LIB_NAME}-static PROPERTIES
    OUTPUT_NAME ${LIB_NAME} # 设置静态库的输出名称（不含后缀）
)

# 添加动态库
add_library(${LIB_NAME}-shared SHARED ${LIB_SRC})
# 设置动态库属性
set_target_properties(${LIB_NAME}-shared PROPERTIES
    OUTPUT_NAME ${LIB_NAME} # 设置动态库的输出名称（不含后缀）
)

# 添加宏定义
target_compile_definitions(${LIB_NAME}-static 
    PUBLIC LINUX
    PUBLIC NET_SDK_SERVER_API
)
target_compile_definitions(${LIB_NAME}-shared  
    PUBLIC LINUX
    PUBLIC NET_SDK_SERVER_API
)
# 添加头文件目录
target_include_directories(${LIB_NAME}-static PUBLIC 
    ${SDK_BUILD_PATH}/include
    ${LIB_INCLUDE}
    ${INCLUDE_SSL}
)
target_include_directories(${LIB_NAME}-shared PUBLIC 
    ${SDK_BUILD_PATH}/include
    ${LIB_INCLUDE}
    ${INCLUDE_SSL}
)

# 添加依赖库
target_link_libraries(${LIB_NAME}-static PRIVATE
    # 第三方库全部引入
    "-Wl,--whole-archive"
    # 系统库采用默认方式
    "-Wl,--no-whole-archive"
    # 系统库
    pthread dl
    ${LIB_CRYPTO}
    ${LIB_SSL}
)
target_link_libraries(${LIB_NAME}-shared PRIVATE
    # 第三方库全部引入
    "-Wl,--whole-archive"
    # 系统库采用默认方式
    "-Wl,--no-whole-archive"
    # 系统库
    pthread dl
    ${LIB_CRYPTO}
    ${LIB_SSL}
)

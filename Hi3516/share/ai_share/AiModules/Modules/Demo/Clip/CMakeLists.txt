# 版本限定
cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 17)            # 设置 C++17 标准
set(CMAKE_CXX_STANDARD_REQUIRED True) # 强制要求 C++17 标准


set(PLATFORM "RK")
set(PLATFORM_SOC "rk3588")

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# 设置程序名称
project("Clip_Demo")

# include(${CMAKE_CURRENT_LIST_DIR}/../../../../../platform/platform.cmake)
set(CMAKE_C_COMPILER aarch64-none-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-none-linux-gnu-g++)
set(CMAKE_AR aarch64-none-linux-gnu-ar)
set(CMAKE_RANLIB aarch64-none-linux-gnu-ranlib)
set(CMAKE_LD aarch64-none-linux-gnu-ld)

include(${CMAKE_CURRENT_LIST_DIR}/../../../../../platform/lib/opencv/opencv.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../../../../platform/lib/rockit/rockit.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../../../../platform/lib/zlog/zlog.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../../../../platform/lib/sqlite3/sqlite3.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../../../../platform/lib/faiss/faiss.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../../Common/FaissDatabase/FaissDatabase.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../ClipImage/ClipImage.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../ClipText/ClipText.cmake)


# 设置生成文件存放路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../output/bin)


# 添加其他源文件与头文件
set(SHARE_SRC_PATH
    ${CMAKE_CURRENT_LIST_DIR}/ClipSqlite
)
set(SHARE_INCLUDE_PATH
    ${CMAKE_CURRENT_LIST_DIR}/ClipSqlite 
)
# 添加共享库的头文件
foreach(item ${SHARE_INCLUDE_PATH})
    include_directories(${item})
endforeach()
# 添加共享库的源文件
foreach(item ${SHARE_SRC_PATH})
    aux_source_directory(${item} SRC_LIST)
endforeach()

# 编译
add_executable(${PROJECT_NAME} main.cpp ${SRC_LIST})

# 链接库路径
target_link_libraries(${PROJECT_NAME}
    librockit
    libopencv
    FaissDatabase_Com
    libsqlite3
    ClipImage_Module
    ClipText_Module
)



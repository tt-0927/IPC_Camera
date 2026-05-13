# 版本限定
cmake_minimum_required(VERSION 3.12)

# 设置程序名称
project("FaissDatabase_Demo")

set(PLATFORM "RK")
set(PLATFORM_SOC "rk3588")

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER aarch64-none-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-none-linux-gnu-g++)
set(CMAKE_AR aarch64-none-linux-gnu-ar)
set(CMAKE_RANLIB aarch64-none-linux-gnu-ranlib)
set(CMAKE_LD aarch64-none-linux-gnu-ld)

# include(${CMAKE_CURRENT_LIST_DIR}/../../../../../platform/platform.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../../../../platform/lib/openblas/openblas.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../../../../platform/lib/faiss/faiss.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../FaissDatabase.cmake)

# 设置生成文件存放路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../output/bin)

# 编译
add_executable(${PROJECT_NAME} main.cpp)


# 链接库路径
target_link_libraries(${PROJECT_NAME}
    libopenblas 
    FaissDatabase_Module
)


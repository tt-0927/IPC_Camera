# 版本限定
cmake_minimum_required(VERSION 3.10.2)

project(Kws)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_C_COMPILER "aarch64-buildroot-linux-gnu-gcc")
set(CMAKE_CXX_COMPILER "aarch64-buildroot-linux-gnu-g++")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_C_FLAGS "-march=armv8-a")
set(CMAKE_CXX_FLAGS "-march=armv8-a")
# cache flags
set(CMAKE_POSITION_INDEPENDENT_COOOE ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC" CACHE STRING "c flags")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC" CACHE STRING "c++ flags")

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/
    ${CMAKE_CURRENT_LIST_DIR}/include)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../ kwsBuild)

add_executable(main ${CMAKE_CURRENT_LIST_DIR}/main.cc ${CMAKE_CURRENT_LIST_DIR}/alsa.cc)

target_link_libraries(main PRIVATE -lasound CpuCAInferenceModel_KWS)

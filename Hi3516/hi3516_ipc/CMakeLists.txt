cmake_minimum_required(VERSION 3.12)

# 必须在 project(ipc)前面
include(${CMAKE_CURRENT_LIST_DIR}/ipc.cmake)

project(ipc)

# 优先查找静态库
# set(CMAKE_FIND_LIBRARY_SUFFIXES .a .so)

# add_compile_options(-fstack-protector-strong -D_FORTIFY_SOURCE=2)

add_subdirectory(main_app)
add_subdirectory(cgi-enc)
add_subdirectory(
    ${IPC_SHARE_PATH}/public_app/daemon
    ${CMAKE_BINARY_DIR}/daemon)
add_subdirectory(
    ${IPC_SHARE_PATH}/public_app/operation
    ${CMAKE_BINARY_DIR}/operation)
add_subdirectory(
    ${IPC_SHARE_PATH}/public_app/record
    ${CMAKE_BINARY_DIR}/record)
add_subdirectory(system_monitor)
add_subdirectory(upgrade)
# example
# add_subdirectory(sample_aidetect)
# message(STATUS "ROOT_PATH = ${ROOT_PATH}")
# add_subdirectory(
#     ${ROOT_PATH}/share/ipc_share/encrypt/gb35114/gmssl/example
#     ${CMAKE_BINARY_DIR}/gmssl_example  # 指定二进制目录（可自定义）
# )

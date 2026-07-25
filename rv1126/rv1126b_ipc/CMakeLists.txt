cmake_minimum_required(VERSION 3.16.2)

# 必须在 project(ipc)前面
include(${CMAKE_CURRENT_LIST_DIR}/ipc.cmake)

project(ipc)

#process
add_subdirectory(
    ${IPC_SHARE_PATH}/public_app/daemon
    ${CMAKE_BINARY_DIR}/daemon)
add_subdirectory(upgrade)
add_subdirectory(
    ${IPC_SHARE_PATH}/public_app/operation
    ${CMAKE_BINARY_DIR}/operation)
add_subdirectory(main_app)
add_subdirectory(
    ${IPC_SHARE_PATH}/public_app/record
    ${CMAKE_BINARY_DIR}/record)

#example（不在当前目录的子目录下，需要包含输入目录和输出目录文件名）
# add_subdirectory(${IPC_SHARE_PATH}/control/business/network/email/example     ${CMAKE_TEMP_PATH}/email_example)
# add_subdirectory(${IPC_SHARE_PATH}/control/business/network/https/example     ${CMAKE_TEMP_PATH}/https_example)
# add_subdirectory(${IPC_SHARE_PATH}/control/business/network/snmp/example      ${CMAKE_TEMP_PATH}/snmp_example)
# add_subdirectory(${IPC_SHARE_PATH}/control/business/system/time/example       ${CMAKE_TEMP_PATH}/time_example)
# add_subdirectory(${IPC_SHARE_PATH}/control/business/network/upnp/example      ${CMAKE_TEMP_PATH}/upnp_example)

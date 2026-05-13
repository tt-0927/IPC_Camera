# sdk_server 模块源文件与头文件
# 使用方式: 在父 CMakeLists.txt 中 include() 本文件，并确保已定义 SRC_LIST

# 源文件目录
set(SDK_SERVER_PATH
    ${CMAKE_CURRENT_LIST_DIR}/src/interface
    ${CMAKE_CURRENT_LIST_DIR}/src/interface/modules
    ${CMAKE_CURRENT_LIST_DIR}/src/service
    ${CMAKE_CURRENT_LIST_DIR}/src/business
    ${CMAKE_CURRENT_LIST_DIR}/src/cb/config
    ${CMAKE_CURRENT_LIST_DIR}/src/cb/device
    ${CMAKE_CURRENT_LIST_DIR}/src/cb/capability
)
set(SDK_SERVER_LIST "")
foreach(item ${SDK_SERVER_PATH})
    aux_source_directory(${item} SDK_SERVER_LIST_TMP)
    list(APPEND SDK_SERVER_LIST ${SDK_SERVER_LIST_TMP})
endforeach()

# 头文件目录
set(SDK_SERVER_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/src/interface
    ${CMAKE_CURRENT_LIST_DIR}/src/interface/modules
    ${CMAKE_CURRENT_LIST_DIR}/src/service
    ${CMAKE_CURRENT_LIST_DIR}/src/business
    ${CMAKE_CURRENT_LIST_DIR}/src/cb/config
    ${CMAKE_CURRENT_LIST_DIR}/src/cb/device
    ${CMAKE_CURRENT_LIST_DIR}/src/cb/capability
)
foreach(item ${SDK_SERVER_INCLUDE})
    include_directories(${item})
endforeach()

list(APPEND SRC_LIST ${SDK_SERVER_LIST})

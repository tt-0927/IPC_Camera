# sdk_share 模块源文件与头文件
# 使用方式: 在父 CMakeLists.txt 中 include() 本文件，并确保已定义 SRC_LIST

# 源文件目录
set(SDK_SHARE_PATH
    ${CMAKE_CURRENT_LIST_DIR}/tools/convert
    ${CMAKE_CURRENT_LIST_DIR}/tools/log/sdk
    ${CMAKE_CURRENT_LIST_DIR}/tools/json
    ${CMAKE_CURRENT_LIST_DIR}/tools/discovery
    # ${CMAKE_CURRENT_LIST_DIR}/tools/json/cJSON
)

foreach(item ${SDK_SHARE_PATH})
    aux_source_directory (${item} SDK_SHARE_LIST)
endforeach()

set(SDK_SHARE_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/tools/log/sdk
    ${CMAKE_CURRENT_LIST_DIR}/tools/convert
    ${CMAKE_CURRENT_LIST_DIR}/tools/json
    ${CMAKE_CURRENT_LIST_DIR}/tools/discovery
    # ${CMAKE_CURRENT_LIST_DIR}/tools/json/cJSON
    ${CMAKE_CURRENT_LIST_DIR}/tools/design
    ${CMAKE_CURRENT_LIST_DIR}/tools/http
)
foreach(item ${SDK_SHARE_INCLUDE})
    include_directories(${item})
endforeach()

list(APPEND SRC_LIST ${SDK_SHARE_LIST})

# sdk_server 模块源文件与头文件
# 使用方式: 在父 CMakeLists.txt 中 include() 本文件，并确保已定义 SRC_LIST

# 源文件目录
set(SDK_SERVER_PATH
    ${CMAKE_CURRENT_LIST_DIR}/src/interface
    ${CMAKE_CURRENT_LIST_DIR}/src/server/http
    ${CMAKE_CURRENT_LIST_DIR}/src/server/session
    ${CMAKE_CURRENT_LIST_DIR}/src/server/server
    ${CMAKE_CURRENT_LIST_DIR}/src/server/BG6_ZHSJ
    ${CMAKE_CURRENT_LIST_DIR}/src/business/Common
    ${CMAKE_CURRENT_LIST_DIR}/src/business/BG6_ZHSJ/BU_SJCL
    ${CMAKE_CURRENT_LIST_DIR}/src/business/BG6_ZHSJ/BU_SJGZ
    ${CMAKE_CURRENT_LIST_DIR}/src/cb/Common
    ${CMAKE_CURRENT_LIST_DIR}/src/cb/BG6_ZHSJ
    ${CMAKE_CURRENT_LIST_DIR}/src/cb/BG6_ZHSJ/BU_SJCL
    ${CMAKE_CURRENT_LIST_DIR}/src/cb/BG6_ZHSJ/BU_SJGZ
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
    ${CMAKE_CURRENT_LIST_DIR}/src/server/http
    ${CMAKE_CURRENT_LIST_DIR}/src/server/session
    ${CMAKE_CURRENT_LIST_DIR}/src/server/server
    ${CMAKE_CURRENT_LIST_DIR}/src/server/BG6_ZHSJ
    ${CMAKE_CURRENT_LIST_DIR}/src/business/Common
    ${CMAKE_CURRENT_LIST_DIR}/src/business/BG6_ZHSJ/BU_SJCL
    ${CMAKE_CURRENT_LIST_DIR}/src/business/BG6_ZHSJ/BU_SJGZ
    ${CMAKE_CURRENT_LIST_DIR}/src/cb/Common
    ${CMAKE_CURRENT_LIST_DIR}/src/cb/BG6_ZHSJ
    ${CMAKE_CURRENT_LIST_DIR}/src/cb/BG6_ZHSJ/BU_SJCL
    ${CMAKE_CURRENT_LIST_DIR}/src/cb/BG6_ZHSJ/BU_SJGZ
)
foreach(item ${SDK_SERVER_INCLUDE})
    include_directories(${item})
endforeach()

list(APPEND SRC_LIST ${SDK_SERVER_LIST})
